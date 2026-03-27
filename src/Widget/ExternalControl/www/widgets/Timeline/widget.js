// 函数级注释：创建EP时间线控件（横向时间轴：多个ID的数值转为时间码，在同一条时间线上按位置显示）
(function () {
  window.EPWidgets = window.EPWidgets || {};

  // 函数级注释：对 items 进行健壮解析（支持数组 / JSON 字符串 / 空值）
  function coerceItems(v, fallbackItems) {
    if (Array.isArray(v)) return v;
    if (typeof v === 'string') {
      try {
        const j = JSON.parse(v);
        if (Array.isArray(j)) return j;
      } catch {}
    }
    return Array.isArray(fallbackItems) ? fallbackItems : [];
  }

  // 函数级注释：注入/更新时间线控件所需的全局样式（避免样式只注入一次导致后续改动不生效）
  function ensureTimelineStyle() {
    const id = 'ep-widget-timeline-style';
    let style = document.getElementById(id);
    if (!style) {
      style = document.createElement('style');
      style.id = id;
      (document.head || document.documentElement).appendChild(style);
    }

    style.textContent = `
.ep-tl-root{
  position:relative;
  width:100%;
  height:100%;
  overflow:hidden;
  background:transparent;
}
.ep-tl-root .ep-tl-canvas{
  position:relative;
  height:100%;
  min-width:100%;
  box-sizing:border-box;
}
.ep-tl-root .ep-tl-base{
  position:absolute;
  left:8px;
  right:8px;
  top:var(--ep-tl-y,70%);
  height:var(--ep-tl-line-w,2px);
  background:var(--ep-tl-line-color,#e5e7eb);
  transform:translateY(-50%);
  pointer-events:none;
  z-index:1;
}
.ep-tl-root .ep-tl-cursor{
  position:absolute;
  top:0;
  bottom:0;
  transform:translateX(-50%);
  pointer-events:none;
  z-index:4;
}
.ep-tl-root .ep-tl-cursor-line{
  position:absolute;
  left:50%;
  top:0;
  bottom:0;
  width:1px;
  background:var(--ep-tl-point-color,#2563eb);
  opacity:.9;
  transform:translateX(-50%);
}
.ep-tl-root .ep-tl-cursor-label{
  position:absolute;
  left:50%;
  top:calc(var(--ep-tl-y,70%) - 12px);
  transform:translate(-50%,-100%);
  padding:2px 6px;
  border-radius:4px;
  background:rgba(17,24,39,.92);
  color:#ffffff;
  font-size:12px;
  line-height:1.4;
  white-space:nowrap;
}
.ep-tl-root .ep-tl-ticks{
  position:absolute;
  inset:0;
  pointer-events:none;
  z-index:2;
}
.ep-tl-root .ep-tl-tick{
  position:absolute;
  top:0;
  bottom:0;
  height:100%;
  transform:translateX(-50%);
  color:var(--ep-tl-text-color,#111827);
  font-size:12px;
  line-height:1;
}
/* 刻度线/文字整体放到轴线下方 */
.ep-tl-root .ep-tl-tick-line{
  position:absolute;
  left:50%;
  top:calc(var(--ep-tl-y,70%) + 0px);
  width:1px;
  height:8px;
  background:var(--ep-tl-line-color,#e5e7eb);
  transform:translateX(-50%);
  opacity:.9;
}
.ep-tl-root .ep-tl-tick-label{
  position:absolute;
  left:50%;
  top:calc(var(--ep-tl-y,70%) + 10px);
  transform:translateX(-50%);
  white-space:nowrap;
  opacity:.85;
}
.ep-tl-root .ep-tl-nodes{
  position:absolute;
  inset:0;
  pointer-events:none;
  z-index:3;
}
.ep-tl-root .ep-tl-node{
  position:absolute;
  top:0;
  height:100%;
  transform:translateX(-50%);
  width:var(--ep-tl-label-max,160px);
  max-width:var(--ep-tl-label-max,160px);
  pointer-events:auto;
  cursor:default;
}
.ep-tl-root .ep-tl-name{
  user-select:none;
  cursor:grab;
  touch-action:none;
}
.ep-tl-root .ep-tl-name:focus{
  outline:none;
  box-shadow:0 0 0 2px var(--ep-tl-point-color,#2563eb);
}
.ep-tl-root .ep-tl-name.is-selected{
  box-shadow:0 0 0 2px var(--ep-tl-point-color,#2563eb);
}
.ep-tl-root .ep-tl-hint{
  position:absolute;
  top:calc(var(--ep-tl-y,70%) - 10px);
  transform:translate(-50%,-100%);
  padding:2px 6px;
  border-radius:4px;
  background:rgba(17,24,39,.92);
  color:#ffffff;
  font-size:12px;
  line-height:1.4;
  pointer-events:none;
  z-index:10;
  white-space:nowrap;
}
/* 节点备注放到轴线下方，leader 从轴线向下连接 */
.ep-tl-root .ep-tl-name{
  position:absolute;
  left:50%;
  top:calc(8px + (var(--ep-tl-lane, 0) * 22px));
  transform:translateX(-50%);
  max-width:100%;
  overflow:hidden;
  text-overflow:ellipsis;
  white-space:nowrap;
  padding:2px 6px;
  border:1px solid var(--ep-tl-label-border,#e5e7eb);
  border-radius:4px;
  background:transparent;
  color:var(--ep-tl-text-color,#111827);
  font-size:inherit;
  line-height:1.4;
  box-sizing:border-box;
}
.ep-tl-root .ep-tl-leader{
  position:absolute;
  left:50%;
  top:calc(34px + (var(--ep-tl-lane, 0) * 22px));
  width:1px;
  height:calc(var(--ep-tl-y,70%) - (42px + (var(--ep-tl-lane, 0) * 22px)));
  min-height:10px;
  background:var(--ep-tl-line-color,#e5e7eb);
  transform:translateX(-50%);
  box-sizing:border-box;
}
.ep-tl-root .ep-tl-dot{
  position:absolute;
  left:50%;
  top:var(--ep-tl-y,70%);
  width:10px;
  height:10px;
  border-radius:50%;
  background:var(--ep-tl-point-color,#2563eb);
  border:2px solid transparent;
  transform:translate(-50%,-50%);
  box-sizing:border-box;
}
`;
  }

  window.EPWidgets.createEPTimelineWidget = function (grid, initialProps = {}, opts = {}) {
    ensureTimelineStyle();

    const defaults = {
      commandId: '/cmd/demo',
      bgColor: 'transparent',
      fontSize: '14',

      // 时间轴配置（范围强制按数据自动计算）
      fps: 25,
      tickStepSec: 15,

      // items: 每个点位一个 CommandID(=后端 address) + 备注
      items: [
        { id: '/cmd/item1', text: 'item 1' },
        { id: '/cmd/item2', text: 'item 2' },
        { id: '/cmd/item3', text: 'item 3' }
      ],

      lineColor: '#e5e7eb',
      pointColor: '#2563eb',
      textColor: '#111827',

      labelBgColor: '#ffffff',
      labelBorderColor: '#e5e7eb',

      axisY: 70,
      labelMaxWidth: 160,
      lineWidth: 2,

      borderColor: '#e5e7eb',
      borderStyle: 'none'
    };

    const coercers = {
      fontSize: (v) => String(v ?? defaults.fontSize),
      fps: (v) => Number(v),
      tickStepSec: (v) => Number(v),
      items: (v) => coerceItems(v, defaults.items),

      axisY: (v) => Number(v),
      labelMaxWidth: (v) => Number(v),
      lineWidth: (v) => Number(v)
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '时间线',
      templatePath: 'widgets/Timeline/widget.html',
      initialProps,
      opts,
      defaultW: 24,
      defaultH: 6,
      defaults,
      coercers,
      // 函数级注释：保持兼容：若仍有外部把 value 写到控件上，这里直接透传（当前时间轴模式不依赖该 value）
      valueMapper(value) {
        return { value: Number(value) };
      },
      // 函数级注释：创建 Vue App（横向时间轴：点=节点，位置=帧数/fps，刻度=tickStepSec）
      appFactory(template) {
        return {
          template,
          data() {
            const rawItems = coerceItems(initialProps.items ?? defaults.items, defaults.items);
            const clonedItems = rawItems.map((it) => ({
              id: (it && (it.id ?? it.commandId) !== undefined) ? String(it.id ?? it.commandId) : '',
              text: (it && (it.text ?? it.name) !== undefined) ? String(it.text ?? it.name) : ''
            }));

            return {
              commandId: initialProps.commandId ?? defaults.commandId,
              value: Number(initialProps.value ?? 0),

              fps: Number(initialProps.fps ?? defaults.fps),
              tickStepSec: Number(initialProps.tickStepSec ?? defaults.tickStepSec),

              items: clonedItems,
              valuesById: {},
              canvasWidth: 0,
              dragging: null,
              dragHint: null,
              selectedId: null,
              __dragStartClientX: 0,
              __dragStarted: false,
              __dragLastSendAt: 0,
              __dragLastSentFrames: null,
              __canvasEl: null,

              lineColor: initialProps.lineColor ?? defaults.lineColor,
              pointColor: (initialProps.pointColor ?? initialProps.activeColor) ?? defaults.pointColor,
              textColor: initialProps.textColor ?? defaults.textColor,

              labelBgColor: initialProps.labelBgColor ?? defaults.labelBgColor,
              labelBorderColor: initialProps.labelBorderColor ?? defaults.labelBorderColor,

              axisY: Number(initialProps.axisY ?? defaults.axisY),
              labelMaxWidth: Number(initialProps.labelMaxWidth ?? defaults.labelMaxWidth),
              lineWidth: Number(initialProps.lineWidth ?? defaults.lineWidth),

              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle
            };
          },
          computed: {
            // 函数级注释：获取安全 fps（避免 0/NaN）
            safeFps() {
              const n = Number(this.fps);
              return Number.isFinite(n) && n > 0 ? n : 25;
            },
            // 函数级注释：获取安全刻度步进（秒）
            safeTickStep() {
              const n = Number(this.tickStepSec);
              return Number.isFinite(n) && n > 0 ? n : 15;
            },
            // 函数级注释：规范化 items（只保留 CommandID+备注；点位数值从 valuesById 读取）
            normalizedItems() {
              const fps = this.safeFps;
              const arr = Array.isArray(this.items) ? this.items : [];
              const out = [];
              arr.forEach((it, idx) => {
                const id = (it && it.id !== undefined && it.id !== null) ? String(it.id) : '';
                const text = (it && it.text !== undefined && it.text !== null) ? String(it.text) : '';
                if (!id && !text) return;
                const v = this.valuesById && id ? this.valuesById[id] : 0;
                const frames = Math.max(0, Math.floor(Number(v) || 0));
                out.push({
                  id,
                  text,
                  frames,
                  sec: frames / fps,
                  key: (id || 'item') + '_' + String(idx)
                });
              });
              return out;
            },
            // 函数级注释：计算时间轴范围（强制按当前数据自动计算；纳入游标位置确保可见）
            rangeSec() {
              const step = this.safeTickStep;
              const fps = this.safeFps;
              const items = this.normalizedItems;
              const cursorFrames = Math.max(0, Math.floor(Number(this.value) || 0));
              const cursorSec = cursorFrames / fps;

              if (!items.length) {
                const start = Math.floor(cursorSec / step) * step;
                const end = start + step;
                return { start, end };
              }

              let min = items[0].sec;
              let max = items[0].sec;
              items.forEach(it => {
                if (it.sec < min) min = it.sec;
                if (it.sec > max) max = it.sec;
              });

              if (Number.isFinite(cursorSec)) {
                if (cursorSec < min) min = cursorSec;
                if (cursorSec > max) max = cursorSec;
              }

              const start = Math.floor(min / step) * step;
              let end = Math.ceil(max / step) * step;
              if (end <= start) end = start + step;
              return { start, end };
            },
            // 函数级注释：游标数据（位置 + 时间码）
            cursor() {
              const fps = this.safeFps;
              const frames = Math.max(0, Math.floor(Number(this.value) || 0));
              const sec = frames / fps;
              const { start, end } = this.rangeSec;
              const span = Math.max(0, end - start);
              const left = span > 0 ? ((sec - start) / span) * 100 : 0;
              const clamped = Math.min(100, Math.max(0, left));
              const label = this.formatHMSF(frames);
              return { frames, sec, left: clamped, label };
            },
            // 函数级注释：生成刻度数据（left% + label）
            ticks() {
              const step = this.safeTickStep;
              const { start, end } = this.rangeSec;
              const span = Math.max(0, end - start);
              const maxTicks = 300;
              const count = Math.min(maxTicks, Math.floor(span / step) + 1);
              const out = [];
              for (let i = 0; i < count; i++) {
                const sec = start + i * step;
                const left = span > 0 ? ((sec - start) / span) * 100 : 0;
                out.push({
                  key: 't_' + String(sec),
                  left,
                  label: this.formatTick(sec)
                });
              }
              return out;
            },
            // 函数级注释：生成点位渲染数据（位置=该 CommandID 的帧数换算秒；自动分配 lane 避免备注重叠）
            nodes() {
              const { start, end } = this.rangeSec;
              const span = Math.max(0, end - start);
              const list = this.normalizedItems;

              const canvasW = Math.max(0, Number(this.canvasWidth) || 0);
              const labelW = Math.max(40, Math.floor(Number(this.labelMaxWidth) || 160));
              const gap = 10;

              const tmp = list.map((it) => {
                const left = span > 0 ? ((it.sec - start) / span) * 100 : 0;
                const x = canvasW > 0 ? (left / 100) * canvasW : left;
                return { it, left, x };
              }).sort((a, b) => a.x - b.x);

              const laneRight = [];
              const laneByKey = {};
              tmp.forEach(({ it, x }) => {
                const half = labelW / 2;
                const leftEdge = x - half;
                let lane = 0;
                while (lane < laneRight.length) {
                  if (leftEdge > laneRight[lane] + gap) break;
                  lane++;
                }
                if (lane === laneRight.length) laneRight.push(x + half);
                else laneRight[lane] = x + half;
                laneByKey[it.key] = lane;
              });

              return list.map((it) => {
                const left = span > 0 ? ((it.sec - start) / span) * 100 : 0;
                const tc = this.formatHMSF(it.frames);
                return {
                  key: it.key,
                  left,
                  lane: laneByKey[it.key] ?? 0,
                  text: it.text,
                  id: it.id,
                  frames: it.frames,
                  title: (it.text ? it.text + '  ' : '') + tc
                };
              });
            },
            // 函数级注释：根容器样式（通过 CSS 变量驱动时间轴布局与颜色）
            rootStyle() {
              const y = Number(this.axisY);
              const labelMax = Number(this.labelMaxWidth);
              const lw = Number(this.lineWidth);
              return {
                width: '100%',
                height: '100%',
                boxSizing: 'border-box',
                borderWidth: '1px',
                borderColor: this.borderColor,
                borderStyle: this.borderStyle,
                borderRadius: '6px',

                '--ep-tl-y': (Number.isFinite(y) ? y : 70) + '%',
                '--ep-tl-line-color': this.lineColor,
                '--ep-tl-point-color': this.pointColor,
                '--ep-tl-text-color': this.textColor,
                '--ep-tl-label-bg': this.labelBgColor,
                '--ep-tl-label-border': this.labelBorderColor,
                '--ep-tl-label-max': (Number.isFinite(labelMax) ? labelMax : 160) + 'px',
                '--ep-tl-line-w': (Number.isFinite(lw) ? lw : 2) + 'px'
              };
            }
          },
          mounted() {
            window.addEventListener('ws-message', this.handleWsMessage);

            const el = this.$el && this.$el.querySelector ? this.$el.querySelector('.ep-tl-canvas') : null;
            this.__canvasEl = el;
            const setW = () => {
              const w = el && el.clientWidth ? el.clientWidth : 0;
              this.canvasWidth = w;
            };
            setW();

            if (typeof ResizeObserver !== 'undefined' && el) {
              this.__tlRo = new ResizeObserver(() => setW());
              try { this.__tlRo.observe(el); } catch {}
            }
          },
          beforeUnmount() {
            window.removeEventListener('ws-message', this.handleWsMessage);
            this.stopDragging();
            if (this.__tlRo) {
              try { this.__tlRo.disconnect(); } catch {}
              this.__tlRo = null;
            }
            this.__canvasEl = null;
          },
          methods: {
            // 函数级注释：两位补零
            pad2(n) {
              return String(Math.max(0, Math.floor(Number(n) || 0))).padStart(2, '0');
            },
            // 函数级注释：格式化刻度（<1小时用 mm:ss，否则 hh:mm:ss）
            formatTick(sec) {
              const s = Math.max(0, Math.floor(Number(sec) || 0));
              const h = Math.floor(s / 3600);
              const m = Math.floor((s % 3600) / 60);
              const ss = s % 60;
              if (h > 0) return this.pad2(h) + ':' + this.pad2(m) + ':' + this.pad2(ss);
              return this.pad2(m) + ':' + this.pad2(ss);
            },
            // 函数级注释：按 fps 将帧数格式化为 hh:mm:ss:ff（用于点位时间码）
            formatHMSF(frames) {
              const fps = this.safeFps;
              const f = Math.max(0, Math.floor(Number(frames) || 0));
              const totalSec = Math.floor(f / fps);
              const ff = f - totalSec * fps;
              const h = Math.floor(totalSec / 3600);
              const m = Math.floor((totalSec % 3600) / 60);
              const ss = totalSec % 60;
              return this.pad2(h) + ':' + this.pad2(m) + ':' + this.pad2(ss) + ':' + this.pad2(ff);
            },
            // 函数级注释：秒数转帧数（按 fps 取整）
            secToFrames(sec) {
              const fps = this.safeFps;
              const s = Math.max(0, Number(sec) || 0);
              return Math.max(0, Math.floor(s * fps));
            },
            // 函数级注释：将 clientX 映射到当前时间轴范围内的位置（ratio/frames/left%）
            clientXToPos(clientX) {
              const el = this.__canvasEl;
              if (!el || !el.getBoundingClientRect) return null;
              const rect = el.getBoundingClientRect();
              const x = (Number(clientX) || 0) - rect.left;
              const w = Math.max(1, rect.width || 1);
              const ratio = Math.min(1, Math.max(0, x / w));
              const { start, end } = this.rangeSec;
              const sec = start + ratio * Math.max(0, end - start);
              const frames = this.secToFrames(sec);
              return { ratio, left: ratio * 100, frames };
            },
            // 函数级注释：将 clientX 映射到当前时间轴范围内的帧数（兼容旧调用）
            clientXToFrames(clientX) {
              const pos = this.clientXToPos(clientX);
              return pos ? pos.frames : null;
            },
            // 函数级注释：选中节点（用于方向键微调）
            selectNode(n) {
              if (!n || !n.id) return;
              this.selectedId = String(n.id);
            },
            // 函数级注释：清理选中态
            clearSelection() {
              this.selectedId = null;
            },
            // 函数级注释：根容器按下时，若未点到文本框则取消选中
            onRootPointerDown(ev) {
              try {
                const t = ev && ev.target ? ev.target : null;
                if (t && t.closest && t.closest('.ep-tl-name')) return;
              } catch {}
              this.clearSelection();
            },
            // 函数级注释：处理节点文本框的按键（左右方向键按帧微调，Shift 加速）
            onNameKeydown(n, ev) {
              if (!n || !n.id || !ev) return;
              const k = ev.key;
              if (k !== 'ArrowLeft' && k !== 'ArrowRight') return;
              ev.preventDefault();

              const id = String(n.id);
              const cur = this.valuesById && this.valuesById[id] !== undefined ? Number(this.valuesById[id]) : Number(n.frames || 0);
              const step = ev.shiftKey ? 10 : 1;
              const next = Math.max(0, Math.floor((cur || 0) + (k === 'ArrowRight' ? step : -step)));

              if (!this.valuesById) this.valuesById = {};
              this.valuesById[id] = next;
              window.EPWidgets.sendCommand(id, String(next));
            },
            // 函数级注释：开始拖拽某个点位（按下后移动超过阈值才真正开始拖拽，避免点击选中时误改帧数）
            startDragging(n, ev) {
              if (!n || !n.id) return;
              if (!ev) return;
              this.selectNode(n);

              try {
                if (ev.currentTarget && ev.currentTarget.focus) {
                  ev.currentTarget.focus({ preventScroll: true });
                }
              } catch {}

              try {
                if (ev.currentTarget && ev.pointerId !== undefined && ev.currentTarget.setPointerCapture) {
                  ev.currentTarget.setPointerCapture(ev.pointerId);
                }
              } catch {}

              this.__dragStartClientX = Number(ev.clientX) || 0;
              this.__dragStarted = false;

              this.dragging = {
                key: n.key,
                id: String(n.id),
                pointerId: ev.pointerId,
                startedAt: Date.now()
              };

              this.dragHint = null;
              this.__dragLastSendAt = 0;
              this.__dragLastSentFrames = null;
              window.addEventListener('pointermove', this.onDragMove, { passive: false });
              window.addEventListener('pointerup', this.onDragEnd, { passive: false });
            },
            // 函数级注释：停止拖拽并清理事件监听
            stopDragging() {
              window.removeEventListener('pointermove', this.onDragMove);
              window.removeEventListener('pointerup', this.onDragEnd);
              this.dragging = null;
              this.dragHint = null;
              this.__dragStarted = false;
              this.__dragStartClientX = 0;
            },
            // 函数级注释：拖拽移动（移动超过阈值才开始；开始后更新本地位置与提示框，并按节流频率发送）
            onDragMove(ev) {
              if (!this.dragging) return;
              if (ev && ev.cancelable) ev.preventDefault();

              if (!this.__dragStarted) {
                const dx = Math.abs((Number(ev.clientX) || 0) - (Number(this.__dragStartClientX) || 0));
                if (dx < 3) return;
                this.__dragStarted = true;
              }

              const pos = this.clientXToPos(ev.clientX);
              if (!pos) return;

              if (!this.valuesById) this.valuesById = {};
              this.valuesById[this.dragging.id] = pos.frames;
              this.dragHint = { left: pos.left, text: this.formatHMSF(pos.frames) + '  (' + String(pos.frames) + ')' };

              const now = Date.now();
              if (now - (this.__dragLastSendAt || 0) < 120) return;
              if (this.__dragLastSentFrames === pos.frames) return;

              this.__dragLastSendAt = now;
              this.__dragLastSentFrames = pos.frames;
              window.EPWidgets.sendCommand(this.dragging.id, String(pos.frames));
            },
            // 函数级注释：拖拽结束（若未真正开始拖拽则不发送；否则确保最终帧数发送一次）
            onDragEnd(ev) {
              if (!this.dragging) return;
              if (ev && ev.cancelable) ev.preventDefault();

              if (!this.__dragStarted) {
                this.stopDragging();
                return;
              }

              const id = this.dragging.id;
              const pos = this.clientXToPos(ev.clientX);
              if (pos) {
                if (!this.valuesById) this.valuesById = {};
                this.valuesById[id] = pos.frames;
                this.dragHint = { left: pos.left, text: this.formatHMSF(pos.frames) + '  (' + String(pos.frames) + ')' };
                window.EPWidgets.sendCommand(id, String(pos.frames));
              }
              this.stopDragging();
            },
            // 函数级注释：处理 ws-message（游标 commandId 与 items[].id 都可能更新）
            handleWsMessage(e) {
              const msg = e && e.detail ? e.detail : null;
              if (!msg || msg.commandId === undefined || msg.value === undefined) return;
              const addr = String(msg.commandId);
              const nv = Math.max(0, Math.floor(Number(msg.value) || 0));

              const cursorAddr = String(this.commandId || '');
              if (cursorAddr && addr === cursorAddr) {
                this.value = nv;
              }

              const arr = Array.isArray(this.items) ? this.items : [];
              const hit = arr.some(it => String(it && it.id !== undefined ? it.id : '') === addr);
              if (!hit) return;
              if (!this.valuesById) this.valuesById = {};
              this.valuesById[addr] = nv;
            }
          }
        };
      }
    });
  };
})();