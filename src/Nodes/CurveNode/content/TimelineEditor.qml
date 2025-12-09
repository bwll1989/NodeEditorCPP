import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


// 函数级注释：纯 QML 时间轴曲线编辑器组件
// - Canvas 绘制网格/坐标轴/折线/Bezier采样/播放头/标记
// - 鼠标交互：选择/拖动/双击删除/点击曲线新增/拖背景平移/滚轮缩放
// - 播放控制：play/pause/seek，Timer 驱动
// - 撤销历史：saveToHistory/resetHistory/undo
// - 数据持久化：exportData()/importData(data)，并通过 dataChanged(json) 自动保存
Item {
    id: root
    width: 800
    height: 300

    // 函数级注释：可配置参数与状态
    property int totalDuration: 3600           // 总时长（秒）
    property int viewportDuration: 30           // 视口时长（秒）
    property int minViewportDuration: 10        // 最小视口范围
    property int maxViewportDuration: 120       // 最大视口范围
    property bool isLoop: false                 // 是否循环播放
    property color lineColor: "#4CAF50"         // 曲线颜色
    property int viewportStart: 0               // 当前视口起始时间（秒）
    property bool isDraggingViewport: false     // 是否在拖拽视口
    property real lastMouseX: 0                 // 上次鼠标 X（用于拖拽）
    property real dragStartTimeUnderMouse: 0    // 视口拖拽起始时鼠标下的时间（秒）
    property real dragStartViewportStart: 0     // 视口拖拽起始时的 viewportStart（秒）
    property int gridRowCount: 6
    property var markers: []                    // 标记时间集合（秒）
    // 函数级注释：撤销历史（最多50条）
    property var history: []
    property int maxHistoryLength: 50

    // 函数级注释：边距设置
    property var margin: ({ left: 10, right: 10, top: 20, bottom: 20 })
    // 函数级注释：控制点数组，type: 0=LINEAR, 1=BEZIER
    property var curves: [
            {
                name: "Curve 1",
                points: [
                    { time: 0,  value: 0.5, type: 0 },
                    { time: 30, value: 0.5, type: 0 }
                ]
            },{
                name: "Curve 2",
                points: [
                    { time: 0,  value: 0.8, type: 0 },
                    { time: 30, value: 0.2, type: 0 }
                ]
            }
        ]
    property int activeCurveIndex: 1
    property var selectedPoint: null            // 选中的控制点
    property int pointRadius: 4                 // 控制点半径（像素）
    // 多曲线随机（稳定）颜色调色板
    property var colorPalette: [
        "#e6194B", "#3cb44b", "#ffe119", "#4363d8", "#f58231",
        "#911eb4", "#42d4f4", "#f032e6", "#bfef45", "#fabed4",
        "#469990", "#dcbeff", "#9A6324", "#800000", "#808000",
        "#000075", "#a9a9a9", "#000000"
    ]
    /*
     * 函数：getCurveColor
     * 目的：为指定索引的曲线提供稳定的随机颜色（基于索引映射）
     * 参数：index - 曲线在 curves 数组中的索引
     * 返回：字符串颜色（Hex），始终为同一索引返回同一颜色
     */
    function getCurveColor(index) {
        return colorPalette[index % colorPalette.length]
    }
    function getCurveLength() {
        var lastPointTime
        if (curves && curves.length > 0) {
            var maxT = 0
            for (var ci = 0; ci < curves.length; ci++) {
                var pts = curves[ci].points || []
                if (pts.length > 0) {
                    var lastT = pts[pts.length - 1].time
                    if (lastT > maxT) maxT = lastT
                }
            }
            lastPointTime = maxT > 0 ? maxT : totalDuration
        } else {
            // 兼容旧结构：仅单曲线 points
            lastPointTime = points.length ? points[points.length - 1].time : totalDuration
        }

        var endTime = Math.min(totalDuration, lastPointTime)
        return endTime
    }
    property var points: (curves[activeCurveIndex] ? curves[activeCurveIndex].points : [])
    // 函数级注释：播放头属性与计时器
    property var playhead: ({
        time: 0,
        color: "#4CAF50",
        width: 1,
        isDragging: false,
        isPlaying: false,
        fps: 25
    })
    Timer {
        id: playTimer
        interval: Math.max(1, 1000 / root.playhead.fps)
        repeat: true
        running: false
        onTriggered: {
            var dt = 1.0 / root.playhead.fps
            var next = root.playhead.time + dt
            var endTime = getCurveLength()

            if (next >= endTime) {
                if (root.isLoop) {
                    seek(0)      // 循环播放，从头开始
                } else {
                    pause()      // 停止播放
                    seek(0) // 定位到结束时间
                    return
                }
            } else {
                seek(next)       // 正常推进
            }
        }
    }
    // 函数级注释：数据变化防抖保存计时器（300ms）
    Timer {
        id: autoSaveTimer
        interval: 300
        repeat: false
        onTriggered: {
            dataChanged(JSON.stringify(exportData()))
        }
    }
    // 函数级注释：数据变化信号（外部接收后更新 savedData）
    signal dataChanged(string json)
    // 函数级注释：播放头变化信号（外部用于同步 UI/C++）
    signal playheadTimeChange(double time,double normTime ,var values, bool isPlaying)
    // 曲线数量变化信号
    signal curveCountChanged()

    function formatTime(seconds) {
        seconds = Math.max(0, Math.floor(seconds))
        var h = Math.floor(seconds / 3600)
        var m = Math.floor((seconds % 3600) / 60)
        var s = seconds % 60
        function pad(n) { return n < 10 ? "0" + n : "" + n }
        if (h > 0) return pad(h) + ":" + pad(m) + ":" + pad(s)
        return pad(m) + ":" + pad(s)
    }
    // 函数级注释：根据时间点计算曲线值（线性/Bezier）
    function getCurrentCurveValue(time) {
        var t = Math.max(0, Math.min(totalDuration, time))
        for (var i = 0; i < points.length - 1; i++) {
            var p1 = points[i], p2 = points[i+1]
            if (t >= p1.time && t <= p2.time) {
                var tt = (t - p1.time) / (p2.time - p1.time)
                if (p1.type === 1 || p2.type === 1) {
                    // 简化 cubic Bezier 采样（无显式控制点时用端点值近似）
                    var cp1y = p1.value, cp2y = p2.value
                    return p1.value * Math.pow(1-tt,3) +
                        cp1y * 3 * tt * Math.pow(1-tt,2) +
                        cp2y * 3 * Math.pow(tt,2) * (1-tt) +
                        p2.value * Math.pow(tt,3)
                } else {
                    return p1.value + (p2.value - p1.value) * tt
                }
            }
        }
        return points.length ? points[points.length-1].value : 0
    }
    // 函数级注释：根据时间点计算曲线值（线性/Bezier）
    function getAllCurveValues(time) {
        var t = Math.max(0, Math.min(totalDuration, time))
        var results = []

        // 兼容：若已是多曲线，则遍历 curves；否则回退到单曲线 points
        var curvesArr = (curves && curves.length) ? curves : [{ points: points }]

        for (var ci = 0; ci < curvesArr.length; ci++) {
            var pts = curvesArr[ci].points || []
            if (!pts || pts.length === 0) {
                results.push(0)
                continue
            }

            // 时间在首尾之外则返回端点值
            if (t <= pts[0].time) {
                results.push(pts[0].value)
                continue
            }
            if (t >= pts[pts.length - 1].time) {
                results.push(pts[pts.length - 1].value)
                continue
            }

            var val = pts[pts.length - 1].value
            for (var i = 0; i < pts.length - 1; i++) {
                var p1 = pts[i], p2 = pts[i + 1]
                if (t >= p1.time && t <= p2.time) {
                    var tt = (t - p1.time) / (p2.time - p1.time)
                    if (p1.type === 1 || p2.type === 1) {
                        // 简化 cubic Bezier 采样（无显式控制点时用端点值近似）
                        var cp1y = p1.value, cp2y = p2.value
                        val = p1.value * Math.pow(1 - tt, 3) +
                              cp1y * 3 * tt * Math.pow(1 - tt, 2) +
                              cp2y * 3 * Math.pow(tt, 2) * (1 - tt) +
                              p2.value * Math.pow(tt, 3)
                    } else {
                        val = p1.value + (p2.value - p1.value) * tt
                    }
                    break
                }
            }
            results.push(val)
        }

        return results
    }
    
    function timeToNormX(t) {
        if (totalDuration <= 0) return 0
        var endTime = getCurveLength();
        var x = t / endTime
        return Math.max(0, Math.min(1, x))
    }
    // 函数级注释：播放头移动/设置（触发重绘与事件）
    function seek(t) {
        var nt = Math.max(0, Math.min(totalDuration, t))
        if (nt !== playhead.time) {
            playhead.time = nt
            canvas.requestPaint()
            var xNorm = timeToNormX(playhead.time)
            var yNorm = getAllCurveValues(playhead.time)
            playheadTimeChange(playhead.time, xNorm, yNorm, playhead.isPlaying)
        }
    }
    // 函数级注释：开始播放
    function play() {
        if (!playhead.isPlaying) {
            playhead.isPlaying = true
            playTimer.start()
            var xNorm = timeToNormX(playhead.time)
            var yNorm = getAllCurveValues(playhead.time)
            playheadTimeChange(playhead.time, xNorm, yNorm, true)
        }
    }
    // 设置循环
    function setIsLoop(isLoop) {
        root.isLoop = isLoop
    }
    // 函数级注释：暂停播放
    function pause() {
        if (playhead.isPlaying) {
            playhead.isPlaying = false
            playTimer.stop()
            var xNorm = timeToNormX(playhead.time)
            var yNorm = getAllCurveValues(playhead.time)
            playheadTimeChange(playhead.time, xNorm, yNorm, false)
        }
    }
    // 函数级注释：导出当前曲线数据（对象）
   function exportData() {
        // 新格式：导出所有曲线
        if (curves && curves.length > 0) {
            var outCurves = curves.map(function (c) {
                var pts = (c && c.points) ? c.points : []
                return {
                    // 保留已有可选字段
                    name: (c && c.name !== undefined) ? c.name : undefined,
                    color: (c && c.color !== undefined) ? c.color : undefined,
                    // 点数据
                    points: pts.map(function (p) {
                        return { time: p.time, value: p.value, type: p.type || 0 }
                    })
                }
            })
            return {
                curves: outCurves,
                activeCurveIndex: (typeof activeCurveIndex === "number") ? activeCurveIndex : 0
            }
        }

        // 旧格式：仅单曲线 points
        return {
            points: points.map(function (p) { return { time: p.time, value: p.value, type: p.type || 0 } })
        }
    }
    function importData(data) {
        if (!data) return false

        // 新格式：{ curves: [...], activeCurveIndex? }
        if (data.curves && data.curves.length >= 0) {
            var normalized = data.curves.map(function (c) {
                var pts = (c && c.points) ? c.points : []
                var normPts = pts.map(function (p) {
                    return { time: p.time, value: p.value, type: p.type || 0 }
                }).sort(function (a, b) { return a.time - b.time })

                var obj = { points: normPts }
                if (c && c.name !== undefined) obj.name = c.name
                if (c && c.color !== undefined) obj.color = c.color
                return obj
            })

            // 不可变赋值以触发绑定刷新
            curves = normalized

            // 校正激活索引
            if (typeof data.activeCurveIndex === "number") {
                activeCurveIndex = Math.max(0, Math.min(curves.length - 1, data.activeCurveIndex))
            } else {
                activeCurveIndex = Math.max(0, Math.min(activeCurveIndex || 0, curves.length - 1))
            }

            // 若存在曲线数量变化信号，则通知外部刷新列表
            if (typeof curveCountChanged === "function") {
                curveCountChanged()
            }

            saveToHistory()
            canvas.requestPaint()
            return true
        }

        // 旧格式：{ points: [...] }
        if (data.points && data.points.length) {
            var normPts2 = data.points.map(function (p) {
                return { time: p.time, value: p.value, type: p.type || 0 }
            }).sort(function (a, b) { return a.time - b.time })

            if (curves && curves.length > 0) {
                // 替换第一条曲线的 points
                curves = curves.map(function (c, i) {
                    if (i === 0) {
                        var obj = { points: normPts2 }
                        if (c && c.name !== undefined) obj.name = c.name
                        if (c && c.color !== undefined) obj.color = c.color
                        return obj
                    }
                    return c
                })
                activeCurveIndex = Math.min(activeCurveIndex || 0, curves.length - 1)
            } else {
                // 创建第一条曲线
                curves = [{ points: normPts2 }]
                activeCurveIndex = 0
            }

            if (typeof curveCountChanged === "function") {
                curveCountChanged()
            }

            saveToHistory()
            canvas.requestPaint()
            return true
        }

        // 数据结构不合法
        return false
    }
    // 函数级注释：保存历史（深拷贝），并触发防抖自动保存
    function saveToHistory() {
        var snap = points.map(function(p) { return { time: p.time, value: p.value, type: p.type } })
        history.push(snap)
        if (history.length > maxHistoryLength) history.shift()
        autoSaveTimer.restart()
    }
    // 函数级注释：重置历史
    function resetHistory() {
        history = []
        saveToHistory()
    }
    // 函数级注释：撤销至上一次状态
    function undo() {
        if (history.length > 1) {
            history.pop()
            points = history[history.length-1].map(function(p){ return { time: p.time, value: p.value, type: p.type } })
            canvas.requestPaint()
            autoSaveTimer.restart()
        }
    }

    // 函数级注释：按 X 坐标映射到时间轴
    function xToTime(x) {
        var chartWidth = canvas.width - margin.left - margin.right
        return viewportStart + ((x - margin.left) / chartWidth) * viewportDuration
    }
    // 坐标换算函数
    function yToValue(y) {
        // 函数级注释：将屏幕 y 转为归一化值（0–1），并做范围钳制
        var chartHeight = canvas.height - margin.top - margin.bottom
        var v = 1 - (y - margin.top) / chartHeight
        if (!isFinite(v)) v = 0
        return Math.max(0, Math.min(1, v))
    }
    // 将归一化值（0–1）映射到屏幕 y（保证与 0–1 值域一致）
    function valueToY(v) {
        var chartHeight = canvas.height - margin.top - margin.bottom
        var vv = Math.max(0, Math.min(1, v))
        return margin.top + (1 - vv) * chartHeight
    }

    // 函数级注释：查找最近控制点（支持“竖直簇”优先，避免横向近点难选）
    // - 遍历所有点，选择最小“度量”者：
    //   * 当鼠标与点在 x 方向非常接近（簇），使用 |dy| 作为优先度量（更容易选上下点）
    //   * 否则使用欧氏距离 d
    // - 命中半径采用较宽容阈值，避免线宽影响点击
    function findNearestPoint(x, y) {
        var chartWidth = canvas.width - margin.left - margin.right
        var hitRadius = Math.max(pointRadius * 2, 10)   // 宽容半径
        var clusterXThreshold = 8                       // 视为“竖直簇”的 x 阈值（像素）
    
        var best = null
        var bestMetric = Infinity
        var bestDx = Infinity
    
        for (var i = 0; i < points.length; i++) {
            var p = points[i]
            var px = margin.left + ((p.time - viewportStart) / viewportDuration) * chartWidth
            var py = valueToY(p.value) // 归一化 0–1
    
            var dx = x - px
            var dy = y - py
            var d  = Math.sqrt(dx * dx + dy * dy)
    
            // 若横向很近（竖直簇），优先比较 |dy|，更容易选上下两点之一
            var metric = (Math.abs(dx) < clusterXThreshold) ? Math.abs(dy) : d
    
            if (metric < bestMetric) {
                bestMetric = metric
                bestDx = Math.abs(dx)
                best = p
            } else if (Math.abs(metric - bestMetric) < 0.5 && Math.abs(dx) < bestDx) {
                // 度量近似相等时，优先选择横向更近的那个
                bestDx = Math.abs(dx)
                best = p
            }
        }
    
        return (bestMetric <= hitRadius) ? best : null
    }

    // 函数级注释：点击曲线近邻处，返回新控制点或 null
    // 命中检测：仅在点击曲线近邻时插入新点
    function checkLineClick(x, y) {
        // 函数级注释：将点击转换为世界坐标（t,v），并以归一化差值/像素阈值判定近邻
        var chartWidth = canvas.width - margin.left - margin.right
        var chartHeight = canvas.height - margin.top - margin.bottom
        var t = viewportStart + (x - margin.left) * viewportDuration / chartWidth
        var v = yToValue(y) // 已归一化 0–1
        if (t <= 0 || t >= totalDuration || v < 0 || v > 1) return null
    
        var expected = getCurrentCurveValue(t)      // 归一化 0–1
        var yExp = valueToY(expected)   // 期望屏幕 y
        var nearByPx = 8                // 像素近邻阈值
    
        if (Math.abs(y - yExp) <= nearByPx) {
            return { time: t, value: v, type: 0 }
        }
        return null
    }
    //添加新的曲线
    function addCurve() {
        var idx = curves.length + 1
        var newCurve = {
            name: "Curve " + idx,
            points: [
                { time: 0,  value: 0.8, type: 0 },
                { time: 30, value: 0.8, type: 0 }
            ]
        }
        curves = curves.concat([newCurve])
        curveCountChanged()
        setActiveCurve(curves.length - 1)
        if (typeof exportData === "function" && typeof dataChanged === "function") {
            dataChanged(JSON.stringify(exportData()))
        }
        // 更新显示与历史
        saveToHistory()

    }
    // 删除曲线
    function removeCurve() {
        if (curves.length <= 1) return
        curves.splice(curves.length - 1, 1)
        curveCountChanged()
        setActiveCurve(curves.length - 1)
        if (typeof exportData === "function" && typeof dataChanged === "function") {
            dataChanged(JSON.stringify(exportData()))
        }
        // 更新显示与历史
        saveToHistory()
        
    }
    // 函数级注释：Canvas 绘制方法
    // 根组件（TimelineEditor）的 Canvas 片段，补充时间轴刻度绘制
    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = canvas.getContext("2d")

            // 背景
            ctx.fillStyle = "#2d2d2d"
            ctx.fillRect(0, 0, canvas.width, canvas.height)

            var chartW = canvas.width - margin.left - margin.right
            var chartH = canvas.height - margin.top - margin.bottom

            // 背景网格（随视口移动/缩放）
            drawGrid(ctx)
            
            drawTimeAxis(ctx)
            drawCurveAndPoints(ctx)
            drawValueAxis(ctx)

            // 播放头
            if (playhead.time >= viewportStart && playhead.time <= viewportStart + viewportDuration) {
                var phx = margin.left + ((playhead.time - viewportStart) / viewportDuration) * chartW
                ctx.strokeStyle = playhead.color; ctx.lineWidth = playhead.width
                ctx.beginPath()
                ctx.moveTo(phx, margin.top)
                ctx.lineTo(phx, margin.top + chartH)
                ctx.stroke()

                // 顶部三角指示
                ctx.fillStyle = playhead.color
                ctx.beginPath()
                ctx.moveTo(phx - 5, margin.top - 8)
                ctx.lineTo(phx + 5, margin.top - 8)
                ctx.lineTo(phx, margin.top)
                ctx.closePath()
                ctx.fill()
            }
        }

        // 函数级注释：滚轮缩放视口范围
        // 删除 WheelHandler（依赖 Qt.labs.handlers），改为 PinchArea + 键盘缩放
        // 函数级注释：PinchArea 支持触控板/触摸缩放，动态调整视口范围，并保持指针下时间不跳变
        PinchArea {
            anchors.fill: parent
            onPinchUpdated: {
                var chartW = canvas.width - margin.left - margin.right
                var mouseX = pinch.center.x
                var timeUnderMouse = viewportStart +
                        ((mouseX - margin.left) / chartW) * viewportDuration

                if (pinch.scale > 1.0) {
                    viewportDuration = Math.max(minViewportDuration, viewportDuration - 5)
                } else if (pinch.scale < 1.0) {
                    viewportDuration = Math.min(maxViewportDuration, viewportDuration + 5)
                }

                viewportStart = Math.max(0, Math.min(
                                          totalDuration - viewportDuration,
                                          timeUnderMouse - (mouseX - margin.left) / chartW * viewportDuration))
                canvas.requestPaint()
            }
        }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        z: 10
        hoverEnabled: true
        preventStealing: true
        focus: true  // 函数级注释：获取键盘焦点，以便处理 +/- 缩放备用逻辑
    
        // 函数级注释：允许左/右键操作；右键用于创建贝塞尔或切换类型
        acceptedButtons: Qt.LeftButton | Qt.RightButton
    
        // 函数级注释：拖拽绘制新点的状态与引用
        property bool isCreatingPoint: false
        property var creatingPoint: null
    
        property double lastClickTime: 0
        property var lastClickPoint: null

        // 将内联 signal handler 改为具名函数引用（不声明形参）
        onPressed: function(mouse) { handleMousePressed(mouse) }
        onPositionChanged: function(mouse) { handleMouseMove(mouse) }
        onReleased: function(mouse) { handleMouseReleased(mouse) }
        onDoubleClicked: function(mouse) { handleMouseDoubleClicked(mouse) }
        onWheel: function(wheel) { handleWheel(wheel) }  // 若 Qt 版本支持 MouseArea.onWheel，将会触发
    
        // 函数级注释：键盘缩放备用逻辑（当 onWheel 不可用时）
        // - 输入：event（KeyEvent）
        // - 行为：按 '+' 放大（减小 viewportDuration），按 '-' 缩小（增大 viewportDuration）
       
    
        // 函数级注释：鼠标按下事件处理
        // - 输入：mouse（MouseEvent），包含 x/y/button/buttons/modifiers
        // - 行为（与 timeline.js 对齐）：
        //   1) 优先选中最近控制点；右键在选中点上切换线性/贝塞尔类型
        //   2) 仅在点击曲线近邻时插入新点（右键为贝塞尔，左键为线性）
        //   3) 命中播放头三角区域进入播放头拖动
        //   4) 其他情况（空白处）开始视口拖动，不创建新点
        function handleMousePressed(mouse) {
            var x = mouse.x, y = mouse.y

            // 控制点优先（支持右键快速切换点类型）
            var pt = findNearestPoint(x, y)
            if (pt) {
                var now = Date.now()
                // 双击删除（首尾点不可删）
                if (now - lastClickTime < 300 && pt === lastClickPoint) {
                    if (pt !== points[0] && pt !== points[points.length - 1]) {
                        var idx = points.indexOf(pt)
                        points.splice(idx, 1)
                        selectedPoint = null
                        saveToHistory()
                        canvas.requestPaint()
                    }
                } else {
                    selectedPoint = pt
                    if (isDraggingViewport) {
                    isDraggingViewport = false
                    saveToHistory()
                    canvas.requestPaint()
                }

                    // 右键切换类型：线性(0) <-> 贝塞尔(1)
                    if (mouse.button === Qt.RightButton) {
                        pt.type = (pt.type === 1 ? 0 : 1)
                        saveToHistory()
                        canvas.requestPaint()
                        lastClickTime = now
                        lastClickPoint = pt
                        return
                    }
                }
                lastClickTime = now
                lastClickPoint = pt
                return
            }

            // 仅当点击曲线近邻时：新增点（与 timeline.js 一致）
            var newPoint = checkLineClick(x, y)
            if (newPoint) {
                newPoint.type = mouse.button === Qt.RightButton ? 1 : 0
                points.push(newPoint)
                points.sort(function(a, b) { return a.time - b.time })
                selectedPoint = newPoint
                isDraggingViewport = false
                saveToHistory()
                canvas.requestPaint()
                return
            }

            // 播放头三角区域命中检测（进入播放头拖动）
            var chartW = canvas.width - margin.left - margin.right
            var phx = margin.left + ((playhead.time - viewportStart) / viewportDuration) * chartW
            if (x >= phx - 5 && x <= phx + 5 && y >= margin.top - 8 && y <= margin.top) {
                playhead.isDragging = true
                return
            }

            // 空白处：开始视口拖动（不创建点），记录锚定的世界坐标（时间）
            isDraggingViewport = true
            lastMouseX = mouse.x
            var chartW2 = canvas.width - margin.left - margin.right
            dragStartTimeUnderMouse = viewportStart + ((x - margin.left) / chartW2) * viewportDuration
            dragStartViewportStart = viewportStart}
    
        // 函数级注释：鼠标移动事件处理
        // - 输入：mouse（MouseEvent）
        // - 行为：
        //   1) 播放头拖动：根据鼠标位置换算时间并调用 seek()
        //   2) 控制点拖动：保持与邻居的时间约束并更新值
        //   3) 视口拖动：按水平位移换算时间偏移并重绘
        function handleMouseMove(mouse) {
            var x = mouse.x, y = mouse.y

            // 播放头拖动
            if (playhead.isDragging) {
                var chartW = canvas.width - margin.left - margin.right
                var t = viewportStart + (x - margin.left) * viewportDuration / chartW
                seek(t)
                return
            }

            // 控制点拖动（包含拖拽创建的新点）
            if (selectedPoint) {
                var t2 = xToTime(x)
                var v2 = yToValue(y)

                if (selectedPoint === points[0]) {
                    // 起点仅纵向移动
                    selectedPoint.value = v2
                } else if (selectedPoint === points[points.length - 1]) {
                    // 终点横纵移动，时间需大于前一邻居
                    var prevTime = points[points.length - 2].time
                    selectedPoint.time = Math.max(prevTime + 1, Math.min(totalDuration, t2))
                    selectedPoint.value = v2
                } else {
                    // 中间点在邻居之间移动（保持时间约束）
                    var idx = points.indexOf(selectedPoint)
                    var prevT = points[idx - 1].time
                    var nextT = points[idx + 1].time
                    selectedPoint.time = Math.max(prevT, Math.min(nextT, t2))
                    selectedPoint.value = v2
                }
                canvas.requestPaint()
                return
            }

            // 视口拖动（锚定鼠标下的世界坐标，缩放下不漂移）
            if (isDraggingViewport) {
                var chartW = canvas.width - margin.left - margin.right
                var newStart = dragStartTimeUnderMouse - ((mouse.x - margin.left) / chartW) * viewportDuration
                viewportStart = Math.max(0, Math.min(totalDuration - viewportDuration, newStart))
                lastMouseX = mouse.x
                canvas.requestPaint()
                return
            }}
    
        // 函数级注释：鼠标释放事件处理
        // - 输入：mouse（MouseEvent）
        // - 行为：
        //   1) 结束播放头拖动
        //   2) 若在拖动控制点，则入历史（触发自动保存）
        //   3) 清理拖拽状态与引用
        function handleMouseReleased(mouse) {
            playhead.isDragging = false
    
            // 结束拖拽绘制：保存历史
            if (selectedPoint) {
                saveToHistory()
            }
            isDraggingViewport = false
            isCreatingPoint = false
            creatingPoint = null
            selectedPoint = null
        }
    
        // 函数级注释：滚轮事件处理
        // - 输入：wheel（WheelEvent），包含 angleDelta/y/x 等
        // - 行为：
        //   1) 以鼠标位置为中心缩放视口（与 timeline.js 一致）
        //   2) 保持鼠标指向时间在缩放后不漂移
        //   3) 约束视口范围在 [minViewportDuration, maxViewportDuration]
        function handleWheel(wheel) {
            // 阻止默认滚动（在使用 Flickable 时尤为重要）
            wheel.accepted = true
    
            var chartW = canvas.width - margin.left - margin.right
            var mouseX = wheel.x
            var timeUnderMouse = viewportStart +
                    ((mouseX - margin.left) / chartW) * viewportDuration
    
            // 缩放步长（秒），与 timeline.js 保持一致为 5s
            var step = 5
            if (wheel.angleDelta.y < 0) {
                // 向上滚动：增大视口范围（缩小）
                viewportDuration = Math.min(maxViewportDuration, viewportDuration + step)
            } else {
                // 向下滚动：减小视口范围（放大）
                viewportDuration = Math.max(minViewportDuration, viewportDuration - step)
            }
    
            // 调整起始时间以保持鼠标所指时间恒定
            viewportStart = Math.max(0, Math.min(
                totalDuration - viewportDuration,
                timeUnderMouse - (mouseX - margin.left) / chartW * viewportDuration
            ))
    
            canvas.requestPaint()
        }
    
        // 函数级注释：鼠标双击事件处理
        // - 输入：mouse（MouseEvent）
        // - 行为：如果双击在图表区域，则将播放头跳转到双击位置（与 timeline.js 一致）
        function handleMouseDoubleClicked(mouse) {
            var chartW = canvas.width - margin.left - margin.right
            var chartH = canvas.height - margin.top - margin.bottom
            var x = mouse.x, y = mouse.y
            var inChart = x >= margin.left && x <= (margin.left + chartW) && y >= margin.top && y <= (margin.top + chartH)
            if (inChart) {
                var t = viewportStart + (x - margin.left) * viewportDuration / chartW
                seek(t)
            }
        }

        Keys.onPressed: function(event) {
            // Ctrl+Z 撤销
            if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Z) {
                undo()
                event.accepted = true
                return
            }

            // 空格键播放/停止
            if (event.key === Qt.Key_Space) {
                if (playhead.isPlaying) {
                    pause()
                } else {
                    play()
                }
                event.accepted = true
                return
            }
            }
        }
    }

    function drawTimeAxis(ctx) {
        // 函数级注释：绘制底部时间刻度线与标签（与网格一致）
        // - 均使用 canvas.width/height 与 margin 计算 chartW/chartH，保证与网格、曲线一致
        // - 刻度线向上绘制到图表区域内；标签绘制在轴线下方
        // - 主/次刻度步长与 drawGrid 相同，随 viewportStart/viewportDuration 自适应
        
        var chartW = canvas.width - margin.left - margin.right
        var chartH = canvas.height - margin.top - margin.bottom
        var axisY = margin.top + chartH  // 等价于 canvas.height - margin.bottom
        
        ctx.save()
        ctx.strokeStyle = "#aaaaaa"
        ctx.fillStyle = "#cccccc"
        ctx.lineWidth = 1
        
        // 轴线（与图表底边重合）
        ctx.beginPath()
        ctx.moveTo(margin.left, axisY)
        ctx.lineTo(margin.left + chartW, axisY)
        ctx.stroke()
        
        // 与网格相同的刻度计算
        var pxPerSec = chartW / viewportDuration
        var intervals = [1, 2, 5, 10, 15, 30, 60, 120, 300, 600]
        var targetPx = 80
        var tickSec = intervals[0]
        for (var i = 0; i < intervals.length; i++) {
            tickSec = intervals[i]
            if (pxPerSec * intervals[i] >= targetPx)
                break
        }
        var minorSec = tickSec / 5
        if (minorSec < 1) minorSec = tickSec / 2
        
        var startSec = Math.floor(viewportStart / minorSec) * minorSec
        var endSec = viewportStart + viewportDuration
        
        // 绘制刻度（向上进入图表区域）
        for (var s = startSec; s <= endSec; s += minorSec) {
            var x = margin.left + (s - viewportStart) * pxPerSec
            var isMajor = Math.abs((s % tickSec)) < 1e-6
            var tickH = isMajor ? 8 : 4
        
            ctx.beginPath()
            ctx.moveTo(x, axisY)
            ctx.lineTo(x, axisY - tickH) // 向上绘制到图表内
            ctx.stroke()
        
            if (isMajor) {
                var label = formatTime(s)
                ctx.fillText(label, x + 2, axisY + 14) // 标签仍在轴线下方
            }
        }
        
        ctx.restore()
    }
    function drawValueAxis(ctx) {
        // 函数级注释：绘制左侧竖轴（归一化 0–1）及标签，固定分割数
        var chartW = canvas.width - margin.left - margin.right
        var chartH = canvas.height - margin.top - margin.bottom
        var axisX = margin.left
    
        ctx.save()
        ctx.strokeStyle = "#aaaaaa"
        ctx.fillStyle = "#cccccc"
        ctx.lineWidth = 1
    
        // 轴线
        ctx.beginPath()
        ctx.moveTo(axisX, margin.top)
        ctx.lineTo(axisX, margin.top + chartH)
        ctx.stroke()
    
        var rows = Math.max(2, gridRowCount)
        for (var r = 0; r <= rows; r++) {
            var v = r / rows
            var y = margin.top + (1 - v) * chartH
            var tickW = 8
    
            ctx.beginPath()
            ctx.moveTo(axisX, y)
            ctx.lineTo(axisX + tickW, y)
            ctx.stroke()
    
            // 标签显示 0.00–1.00
            var label = v.toFixed(2)
            ctx.fillText(label, axisX +8, y + 4)
        }
    
        ctx.restore()
    }
    function drawGrid(ctx) {
        // 函数级注释：绘制随视口移动/缩放的背景网格（时间/归一化值）
        // - 垂直网格：与时间刻度间隔一致
        // - 水平网格：按归一化值域 0–1 的固定分割数 gridRowCount
        // - 绘制区域仅限图表区域
        var chartW = canvas.width - margin.left - margin.right
        var chartH = canvas.height - margin.top - margin.bottom
    
        ctx.save()
        ctx.beginPath()
        ctx.rect(margin.left, margin.top, chartW, chartH)
        ctx.clip()
    
        ctx.strokeStyle = "#3a3a3a"
        ctx.lineWidth = 1
        ctx.beginPath()
    
        // 垂直网格（时间）
        var pxPerSec = chartW / viewportDuration
        var intervals = [1, 2, 5, 10, 15, 30, 60, 120, 300, 600]
        var targetPx = 80
        var tickSec = intervals[0]
        for (var i = 0; i < intervals.length; i++) {
            tickSec = intervals[i]
            if (pxPerSec * intervals[i] >= targetPx)
                break
        }
        var minorSec = tickSec / 5
        if (minorSec < 1) minorSec = tickSec / 2
        var startSec = Math.floor(viewportStart / minorSec) * minorSec
        var endSec = viewportStart + viewportDuration
    
        for (var s = startSec; s <= endSec; s += minorSec) {
            var x = margin.left + (s - viewportStart) * pxPerSec
            ctx.moveTo(x, margin.top)
            ctx.lineTo(x, margin.top + chartH)
        }
    
        // 水平网格（归一化 0–1）
        var rows = Math.max(2, gridRowCount)
        for (var r = 0; r <= rows; r++) {
            var v = r / rows                // 0..1
            var y = margin.top + (1 - v) * chartH
            ctx.moveTo(margin.left, y)
            ctx.lineTo(margin.left + chartW, y)
        }
    
        ctx.stroke()
        ctx.restore()
    }
    function setActiveCurve(index) {
        activeCurveIndex = index
        canvas.requestPaint()
        update()
    }
    /**
     * @brief 绘制所有曲线与当前曲线控制点（同时显示，多色区分）
     * - 遍历 curves，为每条曲线使用自身 color 绘制线段/Bezier
     * - 控制点仅绘制当前激活曲线的点，避免多曲线交互冲突
     */
    function drawCurveAndPoints(ctx) {
        // 归一化曲线与控制点绘制（值域 0–1），裁剪到图表区域
        var chartW = canvas.width - margin.left - margin.right
        var chartH = canvas.height - margin.top - margin.bottom
    
        ctx.save()
        ctx.beginPath()
        ctx.rect(margin.left, margin.top, chartW, chartH)
        ctx.clip()
    
    
        // 循环绘制每条曲线（使用曲线自定义颜色；若缺省则使用 lineColor）
        for (var ci = 0; ci < curves.length; ci++) {
            var curve = curves[ci]
            var pts = curve.points
            var firstVisible = true
    
            // 使用随机（稳定）颜色映射，不再读取 curve.color
            ctx.beginPath()
            ctx.strokeStyle = getCurveColor(ci)
            ctx.lineWidth = 2
      
    
            for (var i = 0; i < pts.length - 1; i++) {
                var p1 = pts[i], p2 = pts[i + 1]
                if (p2.time < viewportStart || p1.time > (viewportStart + viewportDuration)) continue
    
                var x1 = margin.left + ((p1.time - viewportStart) / viewportDuration) * chartW
                var y1 = margin.top + (1 - Math.max(0, Math.min(1, p1.value))) * chartH
                var x2 = margin.left + ((p2.time - viewportStart) / viewportDuration) * chartW
                var y2 = margin.top + (1 - Math.max(0, Math.min(1, p2.value))) * chartH
    
                if (firstVisible) { ctx.moveTo(x1, y1); firstVisible = false }
    
                // Bezier 简化绘制（端点近似）
                if (p1.type === 1 || p2.type === 1) {
                    var steps = 32
                    for (var s = 1; s <= steps; s++) {
                        var tt = s / steps
                        var val = p1.value * Math.pow(1 - tt, 3) +
                                  p1.value * 3 * tt * Math.pow(1 - tt, 2) +
                                  p2.value * 3 * Math.pow(tt, 2) * (1 - tt) +
                                  p2.value * Math.pow(tt, 3)
                        var xx = x1 + (x2 - x1) * tt
                        var yy = margin.top + (1 - Math.max(0, Math.min(1, val))) * chartH
                        ctx.lineTo(xx, yy)
                    }
                } else {
                    ctx.lineTo(x2, y2)
                }
            }
            // 只对该曲线的路径进行描边
            ctx.stroke()
        }
    
        // 控制点仅绘制当前激活曲线的点（交互保持一致）
        ctx.fillStyle = "#FFD54F"
        var actPts = points
        for (var j = 0; j < actPts.length; j++) {
            var p = actPts[j]
            var px = margin.left + ((p.time - viewportStart) / viewportDuration) * chartW
            var py = margin.top + (1 - Math.max(0, Math.min(1, p.value))) * chartH
            ctx.beginPath()
            ctx.arc(px, py, pointRadius, 0, Math.PI * 2)
            ctx.fill()
        }
    
        ctx.restore()                   // 关键：结束裁剪作用，防止影响刻度/播放头
    }
}



