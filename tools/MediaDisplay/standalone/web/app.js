/* ============================================================================
   app.js —— MediaServer 节点编排台（Vue 3 版）

   前端框架 = Vue 3.5（官方 UMD 生产版，web/lib/vue.global.prod.js；MIT）——
   响应式状态 + 模板声明式渲染接管外围 UI（顶栏/左栏/属性面板/折叠分栏）。
   时间轴内核 = vis-timeline 7.7.3（standalone UMD，web/lib/）——
   标尺/多轨/滚轮缩放/片段拖拽/跨轨拖动/播放头游标全由库承担；
   vis 是命令式库，以「静态子树 + 显式 rebuildVis() 同步」方式与 Vue 共存：
   Vue 不 patch #vis-timeline 内部（模板中无任何动态绑定，被编译器静态提升），
   文档 → vis 的同步仍走 rebuildVis 全量重建（visItems/visGroups DataSet）。

   消费后端 REST API（standalone_api.cpp）：
     GET  /api/v1/status                      轮询播放头/状态（250ms）
     GET  /api/v1/timelines                   时间轴列表
     POST /api/v1/timelines                   新建（body = 时间轴文档）
     GET/PUT/DELETE /api/v1/timelines/{id}    文档读/存/删
     POST /api/v1/timelines/{id}/play         启动
     POST /api/v1/timelines/{id}/set-default  开机默认
     POST /api/v1/playback/pause|resume|seek|stop
     GET  /api/v1/library(?q=)                媒体库
     POST /api/v1/library/upload              上传（multipart）

   编辑模型：整文档下载 → 本地编辑（doc ref，深度响应式）→ vis 拖拽落定/
   属性面板修改回写 doc → 400ms 防抖整文档 PUT → 后端重装配 + 活跃热更新
   （applyLocalScene + localSeek 回原播放头）→ 画面即时生效。
   时间映射：1 vis 毫秒 = 1 后端微秒（数值直通，无精度损失）。

   可折叠分栏：左栏（时间轴+媒体库）/右栏（属性）可一键收起，宽度过渡动画，
   状态存 localStorage（ui.left / ui.right）；折叠切换后强制 vis.redraw()
   （vis 只监听 window resize，容器变宽不自动重排）。
   ========================================================================== */
"use strict";

// ---- 全局错误兜底（#err-bar 在 #app 之外——Vue 自身崩溃时也要能显示）---------
// [R8-6] 错误文案本地化映射：网络层原始错误（Failed to fetch 等）直出可读性差
const ERR_HUMAN = [
  [/Failed to fetch|NetworkError|Load failed|network error/i,
   "无法连接显示端，请检查连接"],
];
const humanErr = (msg) => {
  const s = String(msg == null ? "" : msg);
  for (const [re, txt] of ERR_HUMAN) if (re.test(s)) return txt;
  return s;
};
let errBarTimer = 0;
function showFatal(msg) {
  const bar = document.getElementById("err-bar");
  if (!bar) return;
  // 轮询类错误（display 停止后每 250ms/3s 一次 fetch 失败）会无限追加刷屏——
  // 同文本去重，且只保留最近 3 条（2026-11-11 用户报"停止后不断刷错误"）
  const prev = bar.textContent || "";
  const text = humanErr(msg);
  if (prev.includes(text)) return;
  const parts = prev.replace(/^⚠\s*/, "").split(" | ").filter(Boolean);
  parts.push(text);
  bar.hidden = false;
  bar.textContent = "⚠ " + parts.slice(-3).join(" | ");
  // [R8-2] 残留清理：错误条 5s 超时自动消退（新错误会重置计时）
  clearTimeout(errBarTimer);
  errBarTimer = setTimeout(() => { bar.hidden = true; }, 5000);
}
/** [R8-2] 下一次成功操作清残留错误（flushSave 成功/重连时调用） */
function clearErrBar() {
  clearTimeout(errBarTimer);
  const bar = document.getElementById("err-bar");
  if (bar) bar.hidden = true;
}
window.addEventListener("error", (e) => showFatal(`JS 错误: ${e.message} @ ${e.filename?.split("/").pop()}:${e.lineno}`));
window.addEventListener("unhandledrejection", (e) =>
  showFatal(`未处理 Promise 拒绝: ${e.reason && e.reason.message ? e.reason.message : e.reason}`));

// ---- 工具 ------------------------------------------------------------------
const $ = (sel) => document.querySelector(sel);
const api = async (path, opts) => {
  const resp = await fetch(path, opts);
  let body = null;
  try { body = await resp.json(); } catch { /* 非 JSON（静态 404 等） */ }
  if (!resp.ok || (body && body.ok === false)) {
    const msg = body && body.error ? body.error.message : `HTTP ${resp.status}`;
    const e = new Error(msg);
    e.status = resp.status;   // [W1] 409 冲突等按状态码分支处理
    throw e;
  }
  return body ? body.data : null;
};
const fmtTime = (us) => {
  const s = us / 1e6;
  const m = Math.floor(s / 60);
  return `${m}:${(s - m * 60).toFixed(1).padStart(4, "0")}`;
};
const fmtSize = (b) => {
  if (b >= 1 << 30) return (b / (1 << 30)).toFixed(1) + " GB";
  if (b >= 1 << 20) return (b / (1 << 20)).toFixed(1) + " MB";
  if (b >= 1 << 10) return (b / (1 << 10)).toFixed(1) + " KB";
  return b + " B";
};
/** 媒体库文件 → 片段 kind（与 MediaLibrary::detect_kind 一致） */
const kindOf = (name) => {
  const ext = name.toLowerCase().split(".").pop();
  if (["jpg","jpeg","png","webp","bmp","gif","tif","tiff","jxl","avif"].includes(ext)) return "image";
  if (["mp3","wav","flac","aac","m4a","ogg","opus","wma"].includes(ext)) return "audio";
  return "video";
};
const clipKindOf = (clip) => (clip.file ? kindOf(clip.file) : "text");
const clipName = (clip) =>
  clip.file ? clip.file.split("/").pop() : (clip.text || "(文本)");
const clipLabel = (clip) => {
  const n = clipName(clip);
  return n.length > 24 ? n.slice(0, 23) + "…" : n;
};
const enc = (s) => encodeURIComponent(s);
const escapeHtml = (s) =>
  String(s).replace(/[&<>"']/g, (c) =>
    ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
/** 轴标签格式（μs 直通数值）[W10] 与顶栏时间统一口径：去 "s" 后缀（标尺上下文
 * 自明），≥60s 走 m:ss（消除 "1:05s" 读感）。刻度落在 vis 的"现实时刻"网格上
 *（μs↔ms 直通下 hour 网格 = 每 3.6s 一刻），非整秒必须带小数位（3.6s 标成
 * "4" 就是"刻度不准"），整数才省略小数。 */
const fmtAxis = (us) => {
  const s = us / 1e6;
  if (s < 60) return Number.isInteger(s) ? `${s}` : `${s.toFixed(1)}`;
  const m = Math.floor(s / 60);
  return `${m}:${String(Math.round(s - m * 60)).padStart(2, "0")}`;
};

// ---- vis 命令式孤岛（非响应式；Vue 只管外围 UI）------------------------------
let visTimeline = null, visItems = null, visGroups = null;
let visSyncing = false;                     // 程序性重建期间屏蔽事件回写（防回环）
let visFitted = false;                      // 首次渲染自动缩放至全片长
let cursorSyncing = false;                  // 程序性 setCustomTime 期间吞掉 timechanged（防误 seek）
let lastCursorSetUs = -1;
// [W2] 播放头视口跟随（播放中滑出视口自动页翻，用户拖动视口时暂停）
let followProgrammatic = false;             // rangechanged 由程序化 moveTo 发出的标志
let followSuppressUntil = 0;                // 用户视口交互后暂停跟随的截止时刻（performance.now 域）
const playEst = { us: 0, rate: 0, wall: 0, dur: 0 };  // 播放头插值基准（pollStatus 写入）
let bootstrapped = false;                   // 首次列表加载后自动打开一个时间轴

const { createApp, reactive, ref, computed, watch, nextTick, onMounted } = Vue;

const app = createApp({
  setup() {
    // =========================================================================
    // 响应式状态（Vue 接管的 UI 全部由此驱动）
    // =========================================================================
    const timelines = ref([]);                // 列表快照（含 is_active/is_default）
    const currentId = ref(null);              // 编辑中的时间轴 id
    const doc = ref(null);                    // 编辑中的文档（深度响应式）
    const selectedTrack = ref(0);             // 选中轨道下标（追加素材目标）
    const selectedClip = ref(null);           // [trackIdx, clipIdx] | null
    const library = ref([]);
    const status = ref(null);
    const libQuery = ref("");
    const connOk = ref(null);                 // null=连接中 / true / false
    const saveHint = ref("");
    const playheadText = ref("0:00.0");
    // [R8-1] 乐观锁冲突后的"过期"态：云端已被他人修改，本地编辑不再 PUT
    //（未刷新/重载前禁止裸 PUT，防止继续静默丢失）；横幅+一键重载见模板
    const docStale = ref(false);
    // [R8-2] 断连窗口内的编辑计数（重连恢复后用于"断连期间的修改未能保存"提示）
    let offlineLost = 0;

    // ---- 轻量 toast（[W12] 瞬时操作反馈：不占用 saveHint 保存状态流，自动消退）
    let toastTimer = 0;
    function toast(msg, ms = 2600) {
      const el = $("#toast");
      if (!el) return;
      el.textContent = msg;
      el.hidden = false;
      clearTimeout(toastTimer);
      toastTimer = setTimeout(() => { el.hidden = true; }, ms);
    }

    // ---- 可折叠分栏（localStorage 记忆；true=收起）---------------------------
    const ui = reactive({
      left: localStorage.getItem("ui.left") === "1",
      right: localStorage.getItem("ui.right") === "1",
      settings: false,                        // 设置浮层开关（⚙ / sync 强制展开）
      mpcdi: false,                           // 输出校准独立页开关（▣ 输出校准）
    });
    watch(() => [ui.left, ui.right], () => {
      localStorage.setItem("ui.left", ui.left ? "1" : "0");
      localStorage.setItem("ui.right", ui.right ? "1" : "0");
      // vis 只监听 window resize；容器宽度变化须显式重排
      nextTick(() => { if (visTimeline) visTimeline.redraw(); });
    });

    // ---- 设置（2026-11-11 手动模式切换；config.ini 白名单键读写）--------------
    // st 三组键与后端 settingSchema 对齐；表单绑定即编辑，保存走 PUT。
    const st = reactive({ node: {}, display: {}, standalone: {} });
    const settingsHint = ref("");
    const restarting = ref(false);        // display 正在自重启（网页等待重连）
    // 本机显示器枚举（GET /settings 顶层 screens；序 = EnumDisplayMonitors
    // 与 [display] screen / CLI --screen=N 同源）——设置页"显示器"下拉数据源
    const screens = ref([]);
    const modeIsStandalone = computed(
      () => !!status.value && status.value.mode === "standalone");
    const modeIsSync = computed(() =>
      !!status.value && status.value.mode === "sync");
    /** sync 模式：编排面已被 409 门禁收敛——自动弹设置浮层（唯一功能面） */
    watch(() => status.value && status.value.mode, (m) => {
      if (m === "sync" && !ui.settings) ui.settings = true;
    });

    async function loadSettings() {
      try {
        const s = await api("/api/v1/settings");
        st.node = s.node || {};
        st.display = s.display || {};
        st.standalone = s.standalone || {};
        screens.value = Array.isArray(s.screens) ? s.screens : [];
      } catch (e) { settingsHint.value = "读取失败：" + humanErr(e.message); }
    }

    /** 等待 display 自重启完成：轮询 /api/v1/status 直到服务恢复（上限 30s）。
     *  重启链（退出析构→CreateProcessW 拉新→初始化）实测数秒；轮询失败=重启中属预期。 */
    async function waitDisplayBack(deadlineMs = 30000) {
      const t0 = Date.now();
      while (Date.now() - t0 < deadlineMs) {
        await new Promise((r) => setTimeout(r, 500));
        try {
          await api("/api/v1/status");          // 服务回来了
          pollStatus();                          // 立即刷新 mode 等派生 UI
          await new Promise((r) => setTimeout(r, 600));
          return true;
        } catch (e) { /* 仍在重启，继续等 */ }
      }
      return false;
    }

    /** display 自重启（M5 接力：POST system/restart → 优雅退出 → 拉新实例
     *  → 端口零冲突）。POST 返回即旗标已置（进程稍后退出）——连接中断不视为
     *  失败，以服务恢复为准；恢复后自动关设置浮层。 */
    async function restartDisplay() {
      if (restarting.value) return;
      restarting.value = true;
      settingsHint.value = "display 正在重启…（等待新实例就绪）";
      try {
        await api("/api/v1/system/restart", { method: "POST" });
      } catch (e) { /* 进程退出掐断连接=预期，继续等服务回来 */ }
      const ok = await waitDisplayBack();
      restarting.value = false;
      if (ok) {
        ui.settings = false;
        saveHint.value = "display 已重启，新配置已生效";
        refreshTimelines(); refreshLibrary(); refreshMpcdiStatus();
      } else {
        settingsHint.value =
          "重启超时——display 可能未在运行，请手动启动后刷新本页";
      }
    }

    /** 保存设置（全量提交三组；后端白名单过滤+值域校验）。
     *  运行模式变化 → 保存成功后自动走 M5 接力重启（2026-11-12 用户定案，
     *  免手动重启）；其他改动仍需重启生效（可点「重启 display」按钮）。 */
    async function saveSettings() {
      settingsHint.value = "保存中…";
      const modeBefore = status.value && status.value.mode;
      try {
        // [T1] 空值（=「当前未设置」占位）不提交对应字段——后端缺键即跳过，
        // 不再撞"仅支持 standalone/sync"这类半人话报错
        const body = {
          node: {},
          display: { ...st.display },
          standalone: { ...st.standalone },
        };
        if (st.node.mode) body.node.mode = st.node.mode;
        if (!st.display.render_backend) delete body.display.render_backend;
        if (!st.display.hw_accel) delete body.display.hw_accel;
        delete body.display.__stub; delete body.standalone.__stub;
        delete body.node.__stub;
        const r = await api("/api/v1/settings", {
          method: "PUT", headers: { "Content-Type": "application/json" },
          body: JSON.stringify(body),
        });
        if (r.errors) { settingsHint.value = `部分失败: ${r.errors}`; return; }
        const modeChanged = st.node.mode && modeBefore &&
                            st.node.mode !== modeBefore;
        if (modeChanged) await restartDisplay();   // 保存成功→自动重启生效
        else settingsHint.value = "已保存——重启 display 进程后生效（可点「重启 display」）";
      } catch (e) { settingsHint.value = "保存失败：" + humanErr(e.message); }
    }

    // ---- 派生显示 -------------------------------------------------------------
    const activeTl = computed(() =>
      status.value ? timelines.value.find((t) => t.id === status.value.active_timeline) : null);
    const activeName = computed(() =>
      activeTl.value ? (activeTl.value.name || status.value.active_timeline) : "—");
    const durationText = computed(() => fmtTime(activeTl.value ? activeTl.value.duration_us : 0));
    // [W9/V4] 状态文案中文化（fallback 统一「待机」，消除 IDLE/Idle 大小写漂移）
    const STATE_ZH = { Idle: "待机", Playing: "播放中", Paused: "已暂停" };
    const stateName = computed(() => {
      const s = status.value && status.value.state_name;
      return (s && STATE_ZH[s]) || "待机";
    });
    const connText = computed(() =>
      connOk.value === true ? "已连接" : connOk.value === false ? "连接失败" : "连接中…");

    /** 当前选中片段的上下文（属性面板数据源；未选 = null） */
    const selClipData = computed(() => {
      if (!doc.value || !selectedClip.value) return null;
      const [ti, ci] = selectedClip.value;
      const track = doc.value.scene.tracks[ti];
      const clip = track && track.clips[ci];
      if (!clip) return null;
      return {
        ti, ci, clip,
        cw: doc.value.scene.canvas_w || 1920,
        ch: doc.value.scene.canvas_h || 1080,
      };
    });

    /** 播放头可见性（vis 容器 class：非响应式 DOM，走直写） */
    function syncPlayheadVisible() {
      const el = $("#vis-timeline");
      if (!el) return;
      const st = status.value;
      const active = st && st.active_timeline;
      el.classList.toggle("no-playhead", !active || active !== currentId.value);
    }

    // =========================================================================
    // 顶栏：状态轮询 + 播放头本地插值
    // 轮询只做纠偏（250ms）；两次轮询之间用 requestAnimationFrame 按速率推进
    // 播放头（顶栏时间 + vis 游标），视觉 60fps 平滑且 setCustomTime 限 10Hz。
    // =========================================================================
    async function pollStatus() {
      try {
        status.value = await api("/api/v1/status");
        connOk.value = true;
        const st = status.value;
        playEst.us = st.playhead_us || 0;
        playEst.rate = st.state_name === "Playing" ? 1 : 0;   // 后端暂无 rate 字段，Playing 恒 1
        playEst.wall = performance.now();
        playEst.dur = activeTl.value ? activeTl.value.duration_us : 0;
        // 游标只跟随"当前打开的时间轴 = 激活时间轴"；打开别的轴时游标隐藏
        syncPlayheadVisible();
      } catch {
        connOk.value = false;
      }
    }

    function cursorTick() {
      if (status.value) {
        const est = playEst.us + Math.max(0, performance.now() - playEst.wall) * playEst.rate * 1000;
        const clamped = Math.min(Math.max(est, 0), Math.max(playEst.dur, 0));
        playheadText.value = fmtTime(clamped);       // 顶栏时间每帧刷新
        // vis 游标 10Hz 步进（100ms 步长肉眼无感，避免 vis 全量重绘卡顿）
        if (visTimeline && Math.abs(clamped - lastCursorSetUs) >= 100000) {
          lastCursorSetUs = clamped;
          cursorSyncing = true;
          visTimeline.setCustomTime(clamped, "playhead");
          setTimeout(() => { cursorSyncing = false; }, 50); // 兜底：emit 若异步也能被吞
        }
        // [W2] 播放头视口跟随：播放中且用户未在交互视口时，播放头越过
        // 右缘 90% 即页翻一屏（播放头保持在视口内）——拖动/缩放视口后
        // 2s 内不跟随（followSuppressUntil），手动 seek 立即恢复跟随。
        if (visTimeline && playEst.rate === 1 &&
            performance.now() >= followSuppressUntil) {
          const win = visTimeline.getWindow();
          const span = win.end.valueOf() - win.start.valueOf();
          if (clamped > win.end.valueOf() - span * 0.1) {
            const target = clamped + span * 0.1;     // 新视窗：播放头位于左缘 10% 处
            followProgrammatic = true;               // 标记：本次 rangechanged 非用户操作
            visTimeline.moveTo(target, { zoom: false });
            setTimeout(() => { followProgrammatic = false; }, 50);
          }
        }
      }
      requestAnimationFrame(cursorTick);
    }

    /** seek 播放头（乐观更新本地状态，轮询随后确认） */
    async function seekToUs(us) {
      const to = Math.max(0, Math.round(us));
      followSuppressUntil = 0;               // [W2] 手动 seek 立即恢复视口跟随
      try {
        await api("/api/v1/playback/seek", {
          method: "POST", headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ to_us: to }),
        });
        playEst.us = to;                        // 乐观更新插值基准（轮询随后确认）
        playEst.wall = performance.now();
      } catch (err) {
        // [W4] 不再完全静默：一次性提示（拖拽中 timechanged 连发不刷 err-bar）
        if (connOk.value !== false)
          saveHint.value = "定位未生效: " + humanErr(err.message);
      }
    }

    /** 活跃时间轴总时长（游标满刻度；无活跃 = 0） */
    function activeDurUs() {
      return activeTl.value ? activeTl.value.duration_us : 0;
    }

    /**
     * 播放头游标"乐观跳变"：点击/拖拽落点立即写 playEst（tick 下一帧摆过去）。
     * 位置统一由 cursorTick 插值推进（pollStatus 纠偏）。
     */
    function updateCursor(us, dur, opt = {}) {
      if (!visTimeline) return;
      $("#vis-timeline").classList.toggle("no-playhead", !!opt.hidden);
      playEst.us = Math.min(Math.max(us || 0, 0), Math.max(dur, 0));
      playEst.wall = performance.now();
      playEst.rate = 0;                         // 本地落点后停一拍，等下个轮询纠偏
    }

    // =========================================================================
    // vis-timeline 内核（命令式；与 Vue 的边界全在此节）
    // =========================================================================

    /** 创建 vis Timeline 实例（一次）并接线全部交互 */
    function ensureVis() {
      if (visTimeline || !window.vis) return;
      visItems = new vis.DataSet([]);
      visGroups = new vis.DataSet([]);
      visTimeline = new vis.Timeline($("#vis-timeline"), visItems, visGroups, {
        stack: false,                           // NLE 语义：同轨重叠不堆叠
        height: "100%",
        min: 0,
        // 初始视窗 = 0~30s。不设的话 vis 默认定位"现实当前时刻"，在
        // 「1 vis ms = 1 μs」映射下 ≈ 1.7e12，离片段所在的 0~60s 区间
        // 天文数字般遥远——刻度全乱、缩放看不到片段（"时间刻度不对"根因）
        start: 0, end: 30e6,
        selectable: true, multiselect: false,
        zoomable: true, moveable: true,         // 滚轮缩放（围绕鼠标）/ 拖拽平移
        verticalScroll: true,                   // 轨道行超出容器高度时显示垂直滚动条
        horizontalScroll: true,                 // shift+滚轮 = 平移
        zoomFriction: 8,                        // 缩放更缓（每格 ~12%），焦点感更强
        zoomMin: 200000,                        // 视窗下限 0.2s
        showCurrentTime: false,                 // 关闭库自带"现实时刻"线
        type: "range",
        editable: { updateTime: true, updateGroup: true, add: false, remove: false },
        // 轨道名牌 = groupTemplate 构建 DOM（vis 官方扩展点：返回 Element 直接
        // 挂载，绕过内置 XSS 过滤——content 字符串路径会整体转义为纯文本）
        groupTemplate: (group) =>
          buildTrackLabel(Number(group.id != null ? group.id : group.groupId)),
        snap: (date) => Math.round(date.valueOf() / 10000) * 10000,   // 10ms 吸附
        format: {
          minorLabels: (d) => fmtAxis(d.valueOf()),
          majorLabels: () => "",                // 只留一层刻度文字（major 仅保留网格线）
        },
        orientation: { axis: "top", item: "top" },   // 刻度置顶（NLE 惯例）
        margin: { item: { vertical: 2, horizontal: 0 } },   // 行等高 44px（CSS 配合）
      });

      visTimeline.addCustomTime(0, "playhead");

      const evUs = (t) => Math.round(Number(t));

      // 点击标尺 = 定位播放头（仅标尺区）；空白区点击 = 取消选中
      visTimeline.on("click", (props) => {
        if (props.what === "axis") {
          if (!status.value || !status.value.active_timeline || props.time == null) return;
          const t = Math.max(0, evUs(props.time));
          updateCursor(t, activeDurUs());
          seekToUs(t);
          return;
        }
        if (!props.item) {
          selectedClip.value = null;
          visTimeline.setSelection([], { focus: false });
        }
      });
      visTimeline.on("timechanged", (props) => {
        if (props.id !== "playhead" || cursorSyncing) return;   // 吞程序性 setCustomTime
        if (!status.value || !status.value.active_timeline) return;
        seekToUs(Math.max(0, evUs(props.time)));
      });

      // [W2] 视口变更 → 区分程序化跟随（moveTo 发出的）与用户拖拽/缩放：
      // 用户操作暂停跟随 2s（避免"用户拖到别处看，播放头又把视口拉走"），
      // 抑制窗过后恢复自动跟随
      visTimeline.on("rangechanged", () => {
        if (followProgrammatic) return;         // 程序化页翻：非用户操作
        followSuppressUntil = performance.now() + 2000;
      });

      // 素材拖拽到轨道 = 自实现 pointer 拖拽（见 onLibMouseDown）；
      // HTML5 DnD 已弃用——真实浏览器拖拽状态机不可控失效（合成事件链路
      // 通但真实 drop 不触发，2026-11-12 用户实测），自实现全链路可控可测。

      // ---- 滚轮行为（2026-11-12 用户定案）--------------------------------------
      // timeline 内容区：滚轮=围绕鼠标水平缩放（vis 原生在 verticalScroll 开启时
      // 缩放+垂直滚动同时发生=用户报的"不仅缩放还上下滚"——capture 拦截自实现）；
      // shift+滚轮=水平平移（vis 原生 deltaX 分支）；名牌列滚轮=上下滚动轨道并
      // 同步内容区（vis 原生，用户确认正确——不拦截）。标尺=vis 原生缩放。
      const visEl = $("#vis-timeline");
      visEl.addEventListener("wheel", (e) => {
        const inCenter = !!(e.target.closest &&
          e.target.closest(".vis-panel.vis-center"));
        if (!inCenter || e.shiftKey) return;  // 名牌列滚动 / shift平移：vis 原生
        e.preventDefault();
        e.stopPropagation();
        if (!doc.value || !visTimeline) return;
        const centerPanel = visEl.querySelector(".vis-panel.vis-center");
        if (!centerPanel) return;
        const rect = centerPanel.getBoundingClientRect();
        const w = visTimeline.getWindow();
        const s = w.start.valueOf(), en = w.end.valueOf();
        const t = s + (e.clientX - rect.left) / Math.max(rect.width, 1) * (en - s);
        const factor = e.deltaY > 0 ? 1 / 1.12 : 1.12;   // 下滚缩小/上滚放大（每格~12%）
        // animation:false——vis setWindow 默认带动画过渡（异步），滚轮必须即时
        visTimeline.setWindow(t + (s - t) / factor, t + (en - t) / factor,
          { animation: false });
      }, { passive: false, capture: true });

      // 选中片段 → 属性面板（selectedClip 变化 → watch 自动 syncDraft）
      visTimeline.on("select", (props) => {
        if (visSyncing || !props.items.length) return;
        const [ti, ci] = props.items[0].split(":").map(Number);
        selectedClip.value = [ti, ci];
        selectedTrack.value = ti;
      });

      // vis 拖拽/缩放片段落定 → 回写文档（跨轨拖动 = 轨道迁移，id 映射失效须重建）
      visItems.on("update", (_e, props) => {
        if (visSyncing || !doc.value) return;
        let structural = false;
        for (const id of props.items) {
          const [ti, ci] = String(id).split(":").map(Number);
          const track = doc.value.scene.tracks[ti];
          const clip = track && track.clips[ci];
          if (!track || !clip) continue;
          const cur = visItems.get(id);
          clip.start_us = Math.max(0, Math.round(cur.start));
          clip.duration_us = Math.max(100000, Math.round(cur.end - cur.start));
          const nti = Number(cur.group);
          if (Number.isInteger(nti) && nti !== ti && doc.value.scene.tracks[nti]) {
            track.clips.splice(ci, 1);
            doc.value.scene.tracks[nti].clips.push(clip);
            selectedClip.value = null;
            structural = true;
          }
        }
        if (structural) rebuildVis();
        else syncDraft();
        markDirty();
      });

      // 首绘保险（"刷新后时间轴面板空白"根因修复）：
      // vis 构造时把根节点置 visibility:hidden，要等首次完整 _redraw 末尾才恢复。
      // 首绘若因容器尚未完成布局（尺寸 0）提前返回，vis 不会自动重试——
      // 对策：轮询 vis 的 initialDrawDone 标志，未完成就强制补 redraw。
      let drawTries = 0, drawErr = "";
      const firstDraw = () => {
        if (!visTimeline || visTimeline.initialDrawDone || drawTries++ > 60) {
          if (drawErr && visTimeline && !visTimeline.initialDrawDone)
            document.title = "VIS-DRAW-FAIL: " + drawErr;
          return;
        }
        try { visTimeline.redraw(); }
        catch (err) { drawErr = `第${drawTries}次: ${err.message}`; }
        setTimeout(firstDraw, 100);
      };
      setTimeout(firstDraw, 0);
    }

    /** 轨道名牌文字 span（click 选中 / dblclick 原地改名）——buildTrackLabel
     *  与改名 finish 共用，保证就地恢复的 span 与初次构建行为完全一致。 */
    function makeTrackNameSpan(ti) {
      const track = doc.value && doc.value.scene.tracks[ti];
      const name = document.createElement("span");
      name.className = "tname";
      name.dataset.ti = ti;                  // 游离节点时按 ti 重查用
      name.textContent = track ? (track.name || `轨 ${ti + 1}`) : `轨 ${ti + 1}`;
      name.title = "点击选中轨道（追加素材目标）· 双击重命名";
      name.addEventListener("click", (e) => {
        e.stopPropagation();
        if (selectedTrack.value === ti) return;   // 已选中不再重建——双击的
        selectedTrack.value = ti;                 // 第二击若触发 rebuildVis，
        rebuildVis();                             // dblclick 的 target 会成
      });                                         // 游离节点，改名框无处挂载
      name.addEventListener("dblclick", (e) => {
        e.stopPropagation();
        startTrackRenameInline(ti, name);
      });
      return name;
    }

    /**
     * 轨道名牌 DOM 构建（vis groupTemplate 官方扩展点：返回 Element 直接挂载，
     * 绕过内置 XSS 过滤，可挂交互控件）。vis 扩展点无法走 Vue 模板——保留命令式。
     */
    function buildTrackLabel(ti) {
      const wrap = document.createElement("div");
      wrap.className = "grp" + (ti === selectedTrack.value ? " sel" : "");
      const name = makeTrackNameSpan(ti);
      const mk = (act, txt, tip) => {
        const b = document.createElement("button");
        b.textContent = txt; b.title = tip;
        b.addEventListener("click", (e) => {
          e.stopPropagation();
          if (act === "del") deleteTrack(ti);
          else if (act === "up") moveTrack(ti, -1);
          else moveTrack(ti, 1);
        });
        return b;
      };
      const btns = document.createElement("span");
      btns.className = "rowbtns";
      btns.appendChild(mk("up", "▲", "上移轨道"));
      btns.appendChild(mk("down", "▼", "下移轨道"));
      btns.appendChild(mk("del", "✕", "删除轨道"));
      wrap.appendChild(name); wrap.appendChild(btns);
      return wrap;
    }

    /** 文档 → vis 数据全量同步（fit=true 时视窗缩放至全片长；保留用户既有缩放） */
    function rebuildVis(fit = false) {
      if (!visTimeline) return;
      const d = doc.value;
      const groups = [], items = [];
      if (d) {
        d.scene.tracks.forEach((track, ti) => {
          const label = track.name || `轨 ${ti + 1}`;
          groups.push({
            id: ti,
            order: ti,                          // vis 按 order 升序渲染——显示顺序恒=数组顺序
            title: `轨道 ${ti + 1}：${label}`,
          });
          track.clips.forEach((clip, ci) => {
            const dur = Math.max(clip.duration_us, 100000);
            items.push({
              id: `${ti}:${ci}`,
              group: ti,
              start: clip.start_us,
              end: clip.start_us + dur,
              content:
                `<div class="clip-name">${escapeHtml(clipLabel(clip))}</div>` +
                `<div class="clip-time">${fmtTime(clip.start_us)} → ${fmtTime(clip.start_us + clip.duration_us)}</div>` +
                (clip.fade_in_us > 0 ? `<div class="fade-in" style="width:${Math.min(100, clip.fade_in_us / Math.max(clip.duration_us, 1) * 100)}%"></div>` : "") +
                (clip.fade_out_us > 0 ? `<div class="fade-out" style="width:${Math.min(100, clip.fade_out_us / Math.max(clip.duration_us, 1) * 100)}%"></div>` : ""),
              className: "clip-kind " + clipKindOf(clip),
              title: escapeHtml(clipName(clip)),
            });
          });
        });
      }
      visSyncing = true;
      visGroups.clear(); visGroups.add(groups);
      visItems.clear(); visItems.add(items);
      visSyncing = false;
      if (d) {
        visTimeline.setOptions({ max: Math.max(docDurationUs() + 30e6, 60e6) });
        if (!visFitted) {
          visFitted = true;
          visTimeline.setWindow(0, Math.max(docDurationUs() + 5e6, 30e6),
            { animation: false });
        }
        if (selectedClip.value) {
          visTimeline.setSelection(
            [selectedClip.value[0] + ":" + selectedClip.value[1]], { focus: false });
        }
      }
    }

    /** 轨道上下移（层级排布 = 轨道次序；后端按轨道次序推导视觉 z） */
    function moveTrack(ti, dir) {
      const arr = doc.value.scene.tracks;
      const j = ti + dir;
      if (j < 0 || j >= arr.length) return;
      [arr[ti], arr[j]] = [arr[j], arr[ti]];
      if (selectedTrack.value === ti) selectedTrack.value = j;
      else if (selectedTrack.value === j) selectedTrack.value = ti;
      // 选中片段跟着轨道换位（否则面板继续指向旧下标=编辑错轨）
      if (selectedClip.value) {
        if (selectedClip.value[0] === ti) selectedClip.value = [j, selectedClip.value[1]];
        else if (selectedClip.value[0] === j) selectedClip.value = [ti, selectedClip.value[1]];
      }
      rebuildVis(); markDirty();
    }

    /** 删除轨道（片段一并删；下标修正 selectedTrack/selectedClip） */
    function deleteTrack(ti) {
      const track = doc.value.scene.tracks[ti];
      if (!track) return;
      const label = track.name || `轨 ${ti + 1}`;
      if (track.clips.length > 0 &&
          !confirm(`删除轨道「${label}」及其 ${track.clips.length} 个片段？`)) return;
      doc.value.scene.tracks.splice(ti, 1);
      if (selectedClip.value) {
        if (selectedClip.value[0] === ti) selectedClip.value = null;
        else if (selectedClip.value[0] > ti) selectedClip.value[0]--;
      }
      if (selectedTrack.value >= doc.value.scene.tracks.length)
        selectedTrack.value = Math.max(0, doc.value.scene.tracks.length - 1);
      rebuildVis(); markDirty();
    }

    /** 轨道原地编辑（2026-11-12 用户定案：弹窗改原地）——名牌名 span 原地
     *  替换为输入框，Enter/失焦提交、Esc 取消；提交走 markDirty 自动保存。 */
    function startTrackRenameInline(ti, nameEl) {
      const track = doc.value && doc.value.scene.tracks[ti];
      if (!track) return;
      if (!nameEl || !nameEl.isConnected) {  // 双击序列中 DOM 可能已被重建
        nameEl = document.querySelector(     // （如选中态先变）——按 data-ti
          `#vis-timeline .tname[data-ti="${ti}"]`);   // 重查当前名牌节点
        if (!nameEl) return;
      }
      const old = track.name || `轨 ${ti + 1}`;
      const input = document.createElement("input");
      input.className = "inline-rename";
      input.value = old;
      input.title = "Enter 确认 · Esc 或点击其他位置取消";
      nameEl.replaceWith(input);
      input.focus(); input.select();
      let done = false;
      const finish = (commit) => {
        if (done) return;
        done = true;                       // Enter/blur 双触发防重入
        const v = input.value.trim();
        if (commit && v && v !== old) {
          track.name = v;
          markDirty();
          saveHint.value = "已重命名「" + v + "」";
        }
        // 就地恢复名牌 span（替换 input），**不触发 vis DataSet 全量重建**——
        // 改名期间点击其他轨道时 blur(finish) 与 click(选中→rebuildVis) 会
        // 连续两次 clear+add 组数据，与 vis 异步重绘队列撞车 →
        // "A duplicate id was found in the parameter array" / hide 崩溃
        //（2026-11-12 用户实测）。局部替换零 DataSet 操作，彻底避开竞争。
        input.replaceWith(makeTrackNameSpan(ti));
      };
      input.addEventListener("keydown", (e) => {
        e.stopPropagation();               // 不冒泡（全局键盘对 INPUT 已豁免，双保险）
        if (e.key === "Enter") finish(true);
        else if (e.key === "Escape") finish(false);
      });
      // blur = 取消（2026-11-12 用户定案：与时间轴改名一致——
      // 点击其他位置=丢弃未确认的编辑，Enter 才是唯一提交路径）
      input.addEventListener("blur", () => finish(false));
      input.addEventListener("click", (e) => e.stopPropagation());
      input.addEventListener("dblclick", (e) => e.stopPropagation());
    }

    // ---- 时间轴原地改名（span↔input 响应式切换）------------------------------
    const renamingTl = ref(null);          // 正在原地改名的 timeline id | null
    const renameDraft = ref("");           // 编辑缓冲（v-model）

    function startRenameTimeline(t) {
      renamingTl.value = t.id;
      renameDraft.value = t.name || t.id;
      nextTick(() => {
        const input = document.querySelector("#tl-list input.inline-rename");
        if (input) { input.focus(); input.select(); }
      });
    }
    function cancelRenameTimeline() { renamingTl.value = null; }

    /** 提交时间轴改名。当前轴 = 改文档走自动保存；
     *  其他轴 = GET 最新文档改名后 PUT（不动当前编辑上下文）。 */
    async function commitRenameTimeline() {
      const id = renamingTl.value;
      if (!id) return;                     // 已提交过（Enter 后 blur 再入）或已取消
      renamingTl.value = null;
      const t = timelines.value.find((x) => x.id === id);
      const trimmed = renameDraft.value.trim();
      if (!t || !trimmed || trimmed === (t.name || t.id)) return;
      try {
        if (t.id === currentId.value && doc.value) {
          doc.value.name = trimmed;
          markDirty();
        } else {
          const d = await api(`/api/v1/timelines/${enc(t.id)}`);
          d.name = trimmed;
          await api(`/api/v1/timelines/${enc(t.id)}`, {
            method: "PUT", headers: { "Content-Type": "application/json" },
            body: JSON.stringify(d),
          });
        }
        refreshTimelines();
        saveHint.value = "已重命名「" + trimmed + "」";
      } catch (err) { alert("重命名失败：" + humanErr(err.message)); }
    }

    function docDurationUs() {
      let end = 0;
      for (const t of doc.value.scene.tracks)
        for (const c of t.clips)
          end = Math.max(end, c.start_us + c.duration_us);
      return end;
    }

    function docDurationUsOf(track) {
      let end = 0;
      for (const c of track.clips) end = Math.max(end, c.start_us + c.duration_us);
      return end;
    }

    // =========================================================================
    // 属性面板（draft 草稿：输入中不被响应式重置；clip → draft 仅在选中变化时）
    // =========================================================================
    const draft = reactive({
      file: "", text: "", start_s: "", duration_s: "", in_s: "",
      x: "", y: "", w: "", h: "", opacity: "", z: "",
      rotation: "", crop_l: "", crop_r: "", crop_t: "", crop_b: "",
      fade_in_s: "", fade_out_s: "",
    });

    /** 选中片段 → 草稿（同步显示值；坐标=画布绝对像素，w/h=0 显示空=自动适配） */
    function syncDraft() {
      const s = selClipData.value;
      if (!s) return;
      const c = s.clip;
      draft.file = c.file || "";
      draft.text = c.text || "";
      draft.start_s = (c.start_us / 1e6).toFixed(2);
      draft.duration_s = (c.duration_us / 1e6).toFixed(2);
      draft.in_s = (c.in_us / 1e6).toFixed(2);
      draft.x = String(c.x);
      draft.y = String(c.y);
      draft.w = c.w > 0 ? String(c.w) : "";
      draft.h = c.h > 0 ? String(c.h) : "";
      draft.opacity = c.opacity;
      draft.z = c.z;
      draft.rotation = c.rotation || 0;
      draft.crop_l = c.crop_l || 0;
      draft.crop_r = c.crop_r || 0;
      draft.crop_t = c.crop_t || 0;
      draft.crop_b = c.crop_b || 0;
      draft.fade_in_s = (c.fade_in_us / 1e6).toFixed(2);
      draft.fade_out_s = (c.fade_out_us / 1e6).toFixed(2);
    }
    watch(selectedClip, () => nextTick(syncDraft));

    /** 草稿 → 片段（坐标/尺寸=画布绝对像素直存直显；w/h 清空 = 0 = 自动适配） */
    function applyDraftToClip() {
      const s = selClipData.value;
      if (!s) return;
      const clip = s.clip;
      clip.text = draft.text;
      const num = (k) => {
        const v = parseFloat(draft[k]);
        return Number.isFinite(v) ? v : null;
      };
      for (const [k, target] of [["start_s", "start_us"], ["duration_s", "duration_us"], ["in_s", "in_us"]]) {
        const v = num(k);
        if (v !== null) clip[target] = Math.round(v * 1e6);
      }
      for (const k of ["x", "y", "w", "h"]) {
        const raw = String(draft[k]).trim();
        if (raw === "") {
          if (k === "w" || k === "h") clip[k] = 0;
          continue;
        }
        const v = parseFloat(raw);
        if (Number.isFinite(v)) clip[k] = v;   // 绝对像素，无换算
      }
      for (const k of ["opacity", "z", "rotation", "crop_l", "crop_r", "crop_t", "crop_b"]) {
        const v = num(k);
        if (v !== null) clip[k] = v;
      }
      const fi = num("fade_in_s"), fo = num("fade_out_s");
      if (fi !== null) clip.fade_in_us = fi * 1e6;
      if (fo !== null) clip.fade_out_us = fo * 1e6;
    }

    /** 属性输入（实时回写 → vis 条同步 → 自动保存）。
     *  只有 vis 条上显示的字段（时间/文本/淡变）才需要重建 vis；
     *  画布类字段（x/y/w/h/crop/rotation/opacity/z）直接跳过——
     *  逐键全量重建 DataSet 是纯浪费（2026-11-11 性能走查）。 */
    const VIS_FIELDS = new Set(["text", "start_s", "duration_s",
                                "fade_in_s", "fade_out_s"]);
    function onProp(field, ev) {
      if (!doc.value || !selectedClip.value) return;
      draft[field] = ev.target.value;
      applyDraftToClip();
      if (VIS_FIELDS.has(field)) rebuildVis();
      markDirty();
    }

    /** 删除片段 */
    function delClip() {
      if (!selectedClip.value) return;
      const [ti, ci] = selectedClip.value;
      doc.value.scene.tracks[ti].clips.splice(ci, 1);
      selectedClip.value = null;
      rebuildVis(); markDirty();
    }

    // =========================================================================
    // 时间轴列表
    // =========================================================================
    let tlFetchFailed = false;               // 断连检测（恢复后重开当前轴）
    async function refreshTimelines() {
      let list;
      try {
        list = (await api("/api/v1/timelines")) || [];
      } catch {
        // display 停止/重启中：轮询失败属预期——连接状态 chip 已表达，
        // 静默返回（err-bar 刷屏修复，2026-11-11）
        tlFetchFailed = true;
        connOk.value = false;
        return;
      }
      timelines.value = list;
      // display 重启恢复（上一轮失败 + 本轮成功）：当前 doc 与后端状态已
      // 失联（保存/播放全失效）——自动重开当前时间轴，用户无感恢复编辑
      if (tlFetchFailed) {
        tlFetchFailed = false;
        // [R8-2] 恢复提示语义化：替换残留的"自动保存失败: …"误导文案；
        // 断连窗口内有编辑 → 明确告知修改已丢弃（重拉覆盖为服务器版本）
        const lost = offlineLost;
        offlineLost = 0;
        if (currentId.value)
          openTimeline(currentId.value, true).then(() => {
            clearErrBar();
            saveHint.value = lost > 0
              ? `连接已恢复——断连期间的修改未能保存，已恢复为服务器版本`
              : "连接已恢复，已加载最新内容";
          }).catch((e) => {
            saveHint.value = "重开时间轴失败: " + humanErr(e.message);
          });
      }
      // 进页面不再"空白等点击"：优先活跃 → 默认 → 第一个
      if (!bootstrapped && !currentId.value && timelines.value.length) {
        bootstrapped = true;
        const st = status.value;
        const pick = (st && timelines.value.find((t) => t.id === st.active_timeline)) ||
          timelines.value.find((t) => t.is_default) || timelines.value[0];
        openTimeline(pick.id, true).catch((e) => {
          connOk.value = false;
          saveHint.value = "打开时间轴失败: " + e.message;
        });
      }
    }

    async function doPlay(id) {
      try {
        await api(`/api/v1/timelines/${enc(id)}/play`, { method: "POST" });
      } catch (err) {
        alert(`播放「${id}」失败：${err.message}` +
              (String(err.message).includes("未装配场景")
                ? "\n（时间轴没有有效片段——先添加素材并保存）" : ""));
      }
      refreshTimelines();
    }

    /** 停止播放（后端全局 stop：Idle + 清活跃保默认） */
    async function doStop() {
      try {
        await api("/api/v1/playback/stop", { method: "POST" });
      } catch (err) {
        if (connOk.value !== false)             // [W4] 失败可见（250ms 轮询仍兜底纠偏）
          saveHint.value = "停止未生效: " + humanErr(err.message);
      }
      refreshTimelines();
    }

    /** 暂停/继续切换（工具条 ⏸/⏵：按后端状态切 pause/resume） */
    async function doPauseToggle() {
      try {
        if (status.value && status.value.state === 2) {   // 2 = Paused → 续播
          await api("/api/v1/playback/resume", { method: "POST" });
        } else {
          await api("/api/v1/playback/pause", { method: "POST" });
        }
      } catch (err) {
        if (connOk.value !== false)             // [W4] 失败可见
          saveHint.value = "操作未生效: " + humanErr(err.message);
      }
      pollStatus();
    }

    /** 播放/暂停合并切换（2026-11-12 用户定案：三键减两键）：
     *  播放中 → 暂停；暂停中 → 继续；待机 → 播放当前时间轴。 */
    function doPlayPause() {
      const st = status.value && status.value.state;
      if (st === 1 || st === 2) doPauseToggle();          // 1=Playing 2=Paused
      else if (currentId.value) doPlay(currentId.value);
    }

    /** 循环开关（时间轴级属性；改动随自动保存 PUT 热更新） */
    function toggleLoop(e) {
      if (!doc.value) return;
      doc.value.loop = !!e.target.checked;
      markDirty();
    }

    /** [W3] 操作失败 → 人话（404=已被别处删除/不存在） */
    function opFail(prefix, err) {
      const msg = err && err.status === 404
        ? "该时间轴已被删除或不存在" : humanErr(err && err.message);
      saveHint.value = `${prefix}: ${msg}`;
    }

    async function setDefault(id) {
      try {
        await api(`/api/v1/timelines/${enc(id)}/set-default`, { method: "POST" });
      } catch (err) { opFail("设为默认失败", err); }   // [W3] 不再裸奔 err-bar
      refreshTimelines();
    }

    async function deleteTimeline(t) {
      if (!confirm(`删除时间轴「${t.name || t.id}」？`)) return;
      cancelAutoSave();   // 防止删除后防抖定时器再 PUT 已删文档（404 噪音）
      try {
        await api(`/api/v1/timelines/${enc(t.id)}`, { method: "DELETE" });
      } catch (err) { opFail("删除失败", err); return; }   // [W3]
      if (currentId.value === t.id) { currentId.value = null; doc.value = null; rebuildVis(); }
      refreshTimelines();
    }

    async function newTimeline() {
      const name = prompt("时间轴名称：", "新时间轴");
      if (name === null) return;
      const trimmed = name.trim();               // [R8-3] 纯空格名拒绝（与改名路径语义对齐）
      if (!trimmed) { alert("时间轴名称不能为空（纯空格）"); return; }
      const d = {
        name: trimmed, loop: false, stop_position_us: 0,
        scene: { canvas_w: 1920, canvas_h: 1080, tracks: [{ name: "轨 1", clips: [] }] },
      };
      try {
        const data = await api("/api/v1/timelines", {
          method: "POST", headers: { "Content-Type": "application/json" },
          body: JSON.stringify(d),
        });
        currentId.value = data.id;
        await reloadDoc();
        refreshTimelines();
        saveHint.value = "已创建——点击素材添加片段，所有修改自动实时生效";
      } catch (err) { alert("新建失败: " + humanErr(err.message)); }
    }

    /** 导出 .show 工程（M5 工程互通；编辑器可直接打开继续编） */
    async function exportShow(t) {
      try {
        const resp = await fetch(`/api/v1/timelines/${enc(t.id)}/export`);
        if (!resp.ok) {
          const j = await resp.json().catch(() => null);
          throw new Error(j && j.error ? j.error.message : `HTTP ${resp.status}`);
        }
        const blob = await resp.blob();
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = `${t.name || t.id}.show`;
        a.click();
        URL.revokeObjectURL(url);
      } catch (err) { alert("导出失败：" + humanErr(err.message)); }
    }

    /** 导入 .show 工程（multipart；导入后自动打开） */
    async function importShow(e) {
      const f = e.target.files[0];
      e.target.value = "";
      if (!f) return;
      const fd = new FormData();
      fd.append("file", f, f.name);
      try {
        const data = await api("/api/v1/timelines/import", { method: "POST", body: fd });
        await refreshTimelines();
        await openTimeline(data.id);
        saveHint.value = `已导入「${data.name}」（${data.id}）`
            + (data.note ? `｜${data.note}` : "");
      } catch (err) { alert("导入失败：" + humanErr(err.message)); }
    }

    async function openTimeline(id, silent = false) {
      try {
        // 切换前先把上一个时间轴未触发的防抖保存落盘（否则定时器带着新 state 触发会存错）
        if (saveTimer) { cancelAutoSave(); await flushSave(); }
        const d = await api(`/api/v1/timelines/${enc(id)}`);
        if (d && d.scene === null) {
          if (!silent) alert("该时间轴由工程文件直配（只读），请在编辑器端修改后另存");
          return;
        }
        currentId.value = id;
        doc.value = d;
        selectedClip.value = null;
        selectedTrack.value = 0;
        visFitted = false;                      // 换文档后视窗重新缩放至全片长
        if (doc.value.scene.tracks.length === 0) addTrack();
        ensureVis(); rebuildVis();
        refreshTimelines();
      } catch (err) {
        // [W3] 失败不再走全局 err-bar 裸文案（bootstrap 路径同样受益）
        saveHint.value = err && err.status === 404
          ? "打开失败: 该时间轴已被删除或不存在"
          : "打开时间轴失败: " + humanErr(err.message);
      }
    }

    function addTrack() {
      doc.value.scene.tracks.push({ name: `轨 ${doc.value.scene.tracks.length + 1}`, clips: [] });
    }

    /** ＋轨道按钮：新轨自动选中（追加素材目标） */
    function addTrackUi() {
      if (!doc.value) { alert("先选择或新建一个时间轴"); return; }
      addTrack();
      selectedTrack.value = doc.value.scene.tracks.length - 1;
      rebuildVis(); markDirty();
    }

    async function reloadDoc() {
      if (!currentId.value) return;
      doc.value = await api(`/api/v1/timelines/${enc(currentId.value)}`);
      visFitted = false;
      ensureVis(); rebuildVis(); syncDraft();
    }

    // =========================================================================
    // 媒体库
    // =========================================================================
    async function refreshLibrary() {
      try {
        library.value =
          (await api("/api/v1/library" + (libQuery.value ? "?q=" + encodeURIComponent(libQuery.value) : ""))).items || [];
      } catch {
        // display 停止/重启中：静默（连接状态 chip 已表达；err-bar 不刷屏）
      }
    }
    // [W7] 搜索防抖 200ms：逐键全量 GET /library（磁盘扫描）收敛为停顿后一次
    let libDebounceTimer = 0;
    watch(libQuery, () => {
      clearTimeout(libDebounceTimer);
      libDebounceTimer = setTimeout(refreshLibrary, 200);
    });

    // ---- 素材拖拽到轨道（自实现 pointer 拖拽；2026-11-12 重写）-----------------
    // HTML5 DnD 真实拖拽不可控失效（合成事件链路通但真实 drop 不触发）——
    // 自实现全链路可控且可用合成 mouse 事件自测：
    // mousedown 记起点 → mousemove 超 6px 阈值进入拖拽态（ghost 跟随 +
    // vis 内容区高亮）→ mouseup 落点：vis 内容区=添加（落点=片段起点），
    // 其他=取消；Esc 随时取消。双击=追加到选中轨道末尾（防误触的兜底路径）。
    let dndState = null;            // { item, startX, startY, active, ghost }

    /** 落点是否在 vis 内容区（center panel；名牌列 .vis-left 不算） */
    function visDropTargetAt(x, y) {
      const el = document.elementFromPoint(x, y);
      return !!(el && el.closest && el.closest("#vis-timeline .vis-panel.vis-center"));
    }

    /** 素材条目按下：记起点并挂全局 move/up（拖拽可能离开列表区） */
    function onLibMouseDown(e, item) {
      if (e.button !== 0) return;
      dndState = { item, startX: e.clientX, startY: e.clientY, active: false, ghost: null };
      window.addEventListener("mousemove", onDndMove);
      window.addEventListener("mouseup", onDndUp);
    }

    function dndCleanup() {
      if (dndState && dndState.ghost) dndState.ghost.remove();
      const visEl = $("#vis-timeline");
      if (visEl) visEl.classList.remove("drop-hint");
      dndState = null;
      window.removeEventListener("mousemove", onDndMove);
      window.removeEventListener("mouseup", onDndUp);
    }

    function onDndMove(e) {
      if (!dndState) return;
      if (!dndState.active) {
        const dx = e.clientX - dndState.startX, dy = e.clientY - dndState.startY;
        if (dx * dx + dy * dy < 36) return;              // 6px 阈值内=未拖拽
        dndState.active = true;
        const g = document.createElement("div");
        g.className = "dnd-ghost";
        g.textContent = dndState.item.name || dndState.item.id;
        document.body.appendChild(g);
        dndState.ghost = g;
      }
      e.preventDefault();                                 // 阻止拖拽中的文本选择
      dndState.ghost.style.left = e.clientX + 14 + "px";
      dndState.ghost.style.top = e.clientY + 10 + "px";
      $("#vis-timeline").classList.toggle("drop-hint", visDropTargetAt(e.clientX, e.clientY));
    }

    function onDndUp(e) {
      if (!dndState) return;
      const st = dndState;
      dndCleanup();
      if (!st.active) return;                             // 原地点击（≤阈值）：无操作
      if (!doc.value) { toast("请先选择或新建一个时间轴再放置素材"); return; }
      if (!visDropTargetAt(e.clientX, e.clientY)) {
        toast("已取消（素材落在轨道区外）"); return;      // [W12] 瞬时提示不占保存状态流
      }
      const props = visTimeline ? visTimeline.getEventProperties(e) : null;
      const ti = props ? Number(props.group) : NaN;
      if (!Number.isInteger(ti) || !doc.value.scene.tracks[ti]) {
        toast("请放到轨道行上（不是名牌列/标尺）"); return;
      }
      dropClipFromLibrary(st.item, ti, Math.max(0, Math.round(Number(props.time) || 0)));
    }

    /** 媒体库条目 → 拖放到指定轨道指定时刻（落点=片段起点；拖拽主路径终点） */
    function dropClipFromLibrary(item, ti, startUs) {
      const track = doc.value.scene.tracks[ti];
      if (!track) return;
      track.clips.push(makeClipFromLibrary(item, startUs));
      selectedTrack.value = ti;
      rebuildVis(); markDirty();
      if (item.kind !== "image" && !(item.duration_us > 0)) {
        saveHint.value = "已添加（时长保存后自动探测）";
      }
    }

    /** 媒体库条目 → 选中轨道末尾追加（双击兜底；拖拽=任意落点主路径） */
    function appendClipFromLibrary(item) {
      if (!doc.value) { alert("先在左侧选择或新建一个时间轴"); return; }
      let track = doc.value.scene.tracks[selectedTrack.value];
      if (!track) {                            // 选中轨越界 → 补一条并选中它
        addTrack();
        selectedTrack.value = doc.value.scene.tracks.length - 1;
        track = doc.value.scene.tracks[selectedTrack.value];
      }
      track.clips.push(makeClipFromLibrary(item, docDurationUsOf(track)));
      rebuildVis(); markDirty();
      if (item.kind !== "image" && !(item.duration_us > 0)) {
        saveHint.value = "已添加（时长保存后自动探测）";
      }
    }

    /** 库条目 → 新片段（file=库 id（相对 assets_root）；图片 5s；探测失败=0 自动） */
    function makeClipFromLibrary(item, startUs) {
      const dur = item.kind === "image" ? 5e6
        : (item.duration_us > 0 ? item.duration_us : 0);
      return {
        file: item.id, text: "",
        start_us: startUs, duration_us: dur, in_us: 0,
        x: 0, y: 0, w: 0, h: 0, opacity: 1, z: 0,
        rotation: 0, crop_l: 0, crop_r: 0, crop_t: 0, crop_b: 0,
        fade_in_us: 0, fade_out_us: 0,
      };
    }

    /** 添加字幕（文本）片段：追加到选中轨道末尾。
     * 后端 Text 片段走 TextRasterizer 测量，默认样式雅黑 48px 白字；
     * 内容/位置/时长选中后属性面板可改（text 字段已有编辑入口）。 */
    function addTextClip(textOpt) {
      if (!doc.value) { alert("先选择或新建一个时间轴"); return; }
      let text = textOpt;
      if (typeof text !== "string") text = prompt("字幕内容：", "标题文字");
      if (text === null) return;
      text = text.trim();
      if (text === "") { alert("字幕内容不能为空"); return; }
      let track = doc.value.scene.tracks[selectedTrack.value];
      if (!track) {                            // 选中轨越界 → 补一条并选中它
        addTrack();
        selectedTrack.value = doc.value.scene.tracks.length - 1;
        track = doc.value.scene.tracks[selectedTrack.value];
      }
      track.clips.push({
        file: "", text,
        start_us: docDurationUsOf(track), duration_us: 5e6, in_us: 0,
        x: 0, y: 0, w: 0, h: 0, opacity: 1, z: 0,
        rotation: 0, crop_l: 0, crop_r: 0, crop_t: 0, crop_b: 0,
        fade_in_us: 0, fade_out_us: 0,
      });
      rebuildVis(); markDirty();
      saveHint.value = "已添加字幕（默认 5s·左上角，属性面板可改内容/位置/时长）";
    }

    async function uploadFiles(e) {
      const files = [...e.target.files];
      for (const f of files) {
        const fd = new FormData();
        fd.append("file", f, f.name);
        try {
          await api("/api/v1/library/upload", { method: "POST", body: fd });
        } catch (err) { alert(`上传失败 ${f.name}：` + humanErr(err.message)); }
      }
      e.target.value = "";
      refreshLibrary();
    }

    // =========================================================================
    // 自动保存（实时生效）
    // =========================================================================
    const SAVE_DEBOUNCE_MS = 400;
    let saveTimer = 0, saving = false, pendingSave = false;

    function scheduleAutoSave() {
      if (saveTimer) clearTimeout(saveTimer);
      saveTimer = setTimeout(flushSave, SAVE_DEBOUNCE_MS);
    }

    /** 立即保存当前文档（防抖汇合点；保存中再来修改 → 完成后补存一次） */
    async function flushSave() {
      saveTimer = 0;
      if (!currentId.value || !doc.value) return;
      // [R8-1] 过期态：禁止继续裸 PUT（未重载前每次 PUT 必 409，修改必丢）
      if (docStale.value) {
        saveHint.value = "本地修改未同步——云端已被他人修改，请先「刷新加载最新」";
        return;
      }
      if (saving) { pendingSave = true; return; }
      saving = true;
      saveHint.value = "实时生效中…";
      try {
        const r = await api(`/api/v1/timelines/${enc(currentId.value)}`, {
          method: "PUT", headers: { "Content-Type": "application/json" },
          body: JSON.stringify(doc.value),
        });
        // [W1] 乐观锁：保存成功后同步服务端版本号（下次 PUT 携带新 rev）
        if (r && typeof r.rev === "number") doc.value.rev = r.rev;
        saveHint.value = "已实时生效 " + new Date().toLocaleTimeString();
        clearErrBar();                          // [R8-2] 成功操作清错误残留
      } catch (err) {
        if (err && err.status === 409) {
          // [W1][R8-1] 版本冲突：另一窗口已保存——停掉自动保存避免循环 409，
          // 进入"过期"态：横幅常驻 + 一键重载；期间本地编辑标记未同步、禁止 PUT
          cancelAutoSave();
          docStale.value = true;
          saveHint.value = "已被其他窗口修改，保存冲突——本地编辑已暂停同步";
        } else {
          saveHint.value = "自动保存失败: " + humanErr(err.message);
        }
      } finally {
        saving = false;
        if (pendingSave) { pendingSave = false; scheduleAutoSave(); }
      }
    }

    function cancelAutoSave() {
      if (saveTimer) { clearTimeout(saveTimer); saveTimer = 0; }
    }

    function markDirty() {
      // [R8-1] 过期态：不再调度 PUT（防止"修改中…"覆盖冲突提示的幻影编辑），
      // 本地修改仅存于页面——横幅按钮一键重载云端最新
      if (docStale.value) {
        saveHint.value = "本地修改未同步（云端已被他人修改）——请刷新加载最新";
        return;
      }
      // [R8-2] 断连期间编辑不发起注定失败的 PUT：计数并明示"未保存"，
      // 恢复后重载服务器版本（计数用于"断连期间的修改未能保存"提示）
      if (connOk.value === false) {
        offlineLost++;
        saveHint.value = "离线，操作未保存——连接恢复后将恢复为服务器版本";
        return;
      }
      saveHint.value = "修改中…";
      scheduleAutoSave();
    }

    /** [R8-1] 冲突横幅「刷新加载最新」：confirm 后丢弃本地未同步修改，
     * GET 重载当前轴（拿到新 rev + 最新内容），退出过期态恢复可编辑。 */
    async function reloadStaleDoc() {
      if (!currentId.value) return;
      if (!confirm("将丢弃本地未同步的修改，并加载云端最新内容。继续？")) return;
      try {
        docStale.value = false;
        await reloadDoc();
        saveHint.value = "已加载云端最新内容，可继续编辑";
      } catch (err) {
        docStale.value = true;                  // 重载失败维持过期态（禁 PUT）
        saveHint.value = "重载失败: " + humanErr(err.message);
      }
    }

    function forceSave() {
      if (!currentId.value || !doc.value) return;
      cancelAutoSave();
      flushSave();
    }

    // =========================================================================
    // 视图缩放（＋－围绕播放头；⛶ 全览）
    // =========================================================================
    function zoomAroundPlayhead(factor) {
      if (!visTimeline) return;
      const w = visTimeline.getWindow();
      const start = w.start.valueOf(), end = w.end.valueOf();
      const focus = Math.min(Math.max(playEst.us, start), end);   // 焦点=播放头（钳在视窗内）
      visTimeline.setWindow(
        start + (focus - start) * (1 - 1 / factor),
        end - (end - focus) * (1 - 1 / factor), { animation: false });
    }
    function fitAll() {
      if (visTimeline && doc.value)
        visTimeline.setWindow(0, Math.max(docDurationUs(), 30e6), { animation: false });
    }
    const zoomIn = () => zoomAroundPlayhead(1.6);
    const zoomOut = () => zoomAroundPlayhead(1 / 1.6);

    // =========================================================================
    // 启动
    // =========================================================================
    onMounted(() => {
      ensureVis();                              // vis 容器已随 Vue 挂载就位
      rebuildVis();
      refreshTimelines();
      refreshLibrary();
      refreshMpcdiStatus();                     // 启动预拉校准状态→顶栏启用态标识
      setInterval(pollStatus, 250);             // 轮询只做纠偏；平滑走 rAF 插值
      setInterval(refreshTimelines, 3000);
      pollStatus();
      requestAnimationFrame(cursorTick);        // 播放头本地插值循环（60fps 平滑）
      runSelftest();

      // ---- 全局键盘（2026-11-12 用户定案）------------------------------------
      // 空格 = 播放/暂停当前时间轴（任何非输入焦点下恒生效，含按钮聚焦——
      // NLE 惯例，覆盖浏览器"空格激活聚焦按钮"默认行为）；
      // Delete = 删除选中片段（与属性面板"删除片段"按钮同路径）。
      // 焦点在输入控件时完全让位：输入框中的空格/Delete 必须留给文本编辑。
      window.addEventListener("keydown", (e) => {
        const t = e.target;
        const typing = t && (t.tagName === "INPUT" || t.tagName === "TEXTAREA" ||
          t.tagName === "SELECT" || t.isContentEditable);
        if (typing) return;
        if (e.code === "Space") {
          e.preventDefault();                   // 防页面滚动 / 按钮默认点击
          if (doc.value) doPlayPause();
        } else if (e.key === "Delete") {
          if (selectedClip.value) { e.preventDefault(); delClip(); }
        } else if (e.key === "Escape" && dndState) {
          dndCleanup();                         // Esc 取消素材拖拽
          toast("已取消拖拽");                  // [W12] 不占保存状态流
        }
      });
      // 调试钩子：#settings=1 / #mpcdi=1 / #mpcdi=custom 启动即展开对应浮层
      // （headless 截图/走查用）
      if (location.hash === "#settings=1") { ui.settings = true; loadSettings(); }
      if (location.hash.startsWith("#mpcdi=")) {
        ui.mpcdi = true; refreshMpcdi();
        if (location.hash === "#mpcdi=custom") mpcdi.layoutKind = "custom";
      }
    });

    // =========================================================================
    // 自测钩子（headless 排障；URL #selftest=... 自动执行，结果写标题）
    // 用法：http://host/#selftest=track-move 等；结果 SELFTEST-OK/FAIL <case> | ...
    // =========================================================================
    function runSelftest() {
      if (!location.hash.startsWith("#selftest=")) return;
      const caseName = location.hash.slice(10);
      setTimeout(async () => {
        try {
          for (let i = 0; i < 40 && !doc.value; i++) await new Promise((r) => setTimeout(r, 250));
          if (!doc.value) { document.title = "SELFTEST-FAIL: no doc opened"; return; }
          const before = doc.value.scene.tracks.map((t) => t.name).join(">");
          if (caseName === "track-move") {
            moveTrack(doc.value.scene.tracks.length - 1, -1);
            moveTrack(doc.value.scene.tracks.length - 2, 1);   // 移回原位（无副作用）
          }
          else if (caseName === "noop") { /* 只验证渲染 */ }
          else if (caseName === "props") {
            const ti = doc.value.scene.tracks.findIndex((t) => t.clips.length);
            if (ti < 0) throw new Error("no clips");
            selectedClip.value = [ti, 0];
            selectedTrack.value = ti;
            await nextTick(); syncDraft(); await nextTick();   // Vue 渲染 + 草稿同步
            const f = document.querySelector("#prop-form");
            if (!f) throw new Error("no form");
            const cw = doc.value.scene.canvas_w, ch = doc.value.scene.canvas_h;
            const clip = doc.value.scene.tracks[ti].clips[0];
            const got = { x: f.elements["x"].value, y: f.elements["y"].value,
                          w: f.elements["w"].value, h: f.elements["h"].value,
                          rot: f.elements["rotation"].value,
                          cl: f.elements["crop_l"].value };
            // 2026-11-11 绝对像素：面板显示值 = 文档像素值（无换算）
            const want = { x: String(clip.x), w: clip.w > 0 ? String(clip.w) : "" };
            if (got.x !== want.x || got.w !== want.w)
              throw new Error(`draft≠clip: form=(${got.x},${got.w}) clip=(${want.x},${want.w})`);
            document.title = `SELFTEST-OK props | px=(${clip.x},${clip.y},${clip.w},${clip.h})` +
              ` | form=(${got.x},${got.y},${got.w || "auto"},${got.h || "auto"})` +
              ` | rot=${got.rot} cropL=${got.cl} | canvas=${cw}x${ch}`;
            return;
          }
          else if (caseName === "drop") {
            // 自实现拖拽全链路：mousedown(素材) → mousemove(>6px，到 vis 中心)
            // → mouseup(落点) → 断言片段插入。验证 pointer 拖拽 + 落点解析。
            // 无副作用：断言后按引用 diff 删除刚插入的片段（曾污染真实工程）。
            for (let i = 0; i < 40 && !library.value.length; i++)
              await new Promise((r) => setTimeout(r, 250));
            const li = document.querySelector("#lib-list li");
            if (!li) throw new Error("no library item");
            const visEl2 = document.querySelector("#vis-timeline");
            const rect = visEl2.getBoundingClientRect();
            const cx = rect.left + (rect.width - 250) / 2, cy = rect.top + 120;
            const n0 = doc.value.scene.tracks.reduce((s, t) => s + t.clips.length, 0);
            const before = new Set();               // 引用快照（插入=新引用）
            doc.value.scene.tracks.forEach((t) => t.clips.forEach((c) => before.add(c)));
            const md = (type, x, y, target) => target.dispatchEvent(
              new MouseEvent(type, { bubbles: true, cancelable: true,
                                     clientX: x, clientY: y, button: 0 }));
            md("mousedown", rect.left + 20, rect.top + 20, li);   // 按下素材条目
            md("mousemove", (rect.left + 20 + cx) / 2, (rect.top + 20 + cy) / 2, window);
            md("mousemove", cx, cy, window);                       // 进入拖拽态
            md("mouseup", cx, cy, window);                         // vis 内容区松手
            const n1 = doc.value.scene.tracks.reduce((s, t) => s + t.clips.length, 0);
            if (n1 !== n0 + 1) throw new Error(`拖拽未插入片段: ${n0}→${n1}`);
            let at = null;                          // 找到新增项并删除还原
            doc.value.scene.tracks.forEach((t, ti) => t.clips.forEach((c, ci) => {
              if (!before.has(c)) at = [ti, ci];
            }));
            if (at) {
              doc.value.scene.tracks[at[0]].clips.splice(at[1], 1);
              rebuildVis(); cancelAutoSave(); flushSave();
            }
            document.title = "SELFTEST-OK drop | +1 已还原 | clips=" +
              doc.value.scene.tracks.map((t) => `${t.name}:${t.clips.length}`).join(",");
            return;
          }
          else if (caseName === "collapse") {
            // 侧栏折叠验证。virtual-time 快进不走 CSS compositor 过渡（宽度冻结
            // 在起点）——临时禁用过渡让 width 立即到位，量得的是布局真值。
            const sb = document.querySelector("#sidebar");
            const pr = document.querySelector("#props");
            sb.style.transition = "none"; pr.style.transition = "none";
            ui.left = true;
            await nextTick();
            await new Promise((r) => setTimeout(r, 80));
            const w1 = sb.getBoundingClientRect().width;
            const hasCls = sb.classList.contains("collapsed");
            ui.left = false;
            await nextTick();
            await new Promise((r) => setTimeout(r, 80));
            const w2 = sb.getBoundingClientRect().width;
            sb.style.transition = ""; pr.style.transition = "";
            if (!hasCls || w1 > 30 || w2 < 200)
              throw new Error(`cls=${hasCls} width collapsed=${w1} expanded=${w2}`);
            document.title = `SELFTEST-OK collapse | left: ${w1}→${w2} | props sync ok`;
            return;
          }
          else if (caseName === "layout-probe") {
            // 布局量测：侧栏两个 panel 的实际几何（排障用）
            const sb = document.querySelector("#sidebar");
            const ps = [...document.querySelectorAll("#sidebar .panel")].map((p) => {
              const r = p.getBoundingClientRect();
              return `${(p.querySelector("h2") || {}).textContent || "?"}:${Math.round(r.top)}+${Math.round(r.height)}`;
            });
            const si = document.querySelector("#sidebar .side-inner").getBoundingClientRect();
            document.title = `LAYOUT | ${ps.join(" | ")} | sideH=${Math.round(sb.getBoundingClientRect().height)} innerH=${Math.round(document.querySelector("#sidebar .side-inner").getBoundingClientRect().height)}`;
            return;
          }
          else if (caseName === "add-text") {
            // 字幕入口验证：直传内容（headless prompt 返回 null）追加到选中轨。
            // 无副作用：断言后删除刚追加的片段（曾遗留 SELFTEST字幕 在用户工程）。
            const n0 = doc.value.scene.tracks.reduce((s, t) => s + t.clips.length, 0);
            addTextClip("SELFTEST字幕");
            const added = doc.value.scene.tracks[selectedTrack.value].clips;
            const c = added[added.length - 1];
            if (!c || c.text !== "SELFTEST字幕" || c.file !== "")
              throw new Error("text clip not appended");
            added.pop();                            // 还原
            rebuildVis(); cancelAutoSave(); flushSave();
            document.title = `SELFTEST-OK add-text | clips=${n0}→${n0 + 1} 已还原` +
              ` | last="${c.text}" dur=${c.duration_us}`;
            return;
          }
          else if (caseName === "loop") {
            // 循环开关验证：翻转 doc.loop + PUT 落盘（toggleLoop 模拟 change）。
            // 无副作用：断言后翻回原值再落盘（曾把用户工程 loop 打开未还原）。
            const before = !!doc.value.loop;
            toggleLoop({ target: { checked: !before } });
            await new Promise((r) => setTimeout(r, 700));   // 等防抖 PUT 完成
            const back = await api(`/api/v1/timelines/${enc(currentId.value)}`);
            const saved = !!back.loop;
            if (saved !== !before) throw new Error(`loop 落盘不符: doc=${doc.value.loop} server=${saved}`);
            toggleLoop({ target: { checked: before } });    // 翻回还原
            await new Promise((r) => setTimeout(r, 700));
            const back2 = await api(`/api/v1/timelines/${enc(currentId.value)}`);
            if (!!back2.loop !== before)
              throw new Error(`loop 还原失败: server=${back2.loop}`);
            document.title = `SELFTEST-OK loop | ${before}→${!before}→还原 | server=${saved}`;
            return;
          }
          else if (caseName === "stale-409") {
            // [R8-1] 409 过期态闭环（全 mock，不碰真实数据）：PUT 409 →
            // stale 置位 + 横幅可见 → 过期态编辑不再发 PUT → 一键重载恢复。
            if (!currentId.value || !doc.value) throw new Error("no doc");
            const realFetch = window.fetch, realConfirm = window.confirm;
            let putCalls = 0;
            const conflictResp = () => new Response(JSON.stringify(
              { ok: false, error: { code: "CONFLICT",
                message: "文档已被其他窗口修改（rev 冲突），请刷新后重试" } }),
              { status: 409, headers: { "Content-Type": "application/json" } });
            const fresh = JSON.parse(JSON.stringify(doc.value));
            fresh.rev = 99; fresh.name = "云端最新";
            const freshResp = () => new Response(
              JSON.stringify({ ok: true, data: fresh }),
              { status: 200, headers: { "Content-Type": "application/json" } });
            window.fetch = async (path, opts) => {
              const p = String(path);
              if (p === `/api/v1/timelines/${enc(currentId.value)}` &&
                  opts && opts.method === "PUT") { putCalls++; return conflictResp(); }
              if (p === `/api/v1/timelines/${enc(currentId.value)}`)
                return freshResp();
              return realFetch(path, opts);
            };
            try {
              await flushSave();                       // → 409 → stale
              if (!docStale.value) throw new Error("stale 未置位");
              await nextTick();
              if (!document.querySelector("#stale-banner"))
                throw new Error("冲突横幅未显示");
              const put1 = putCalls;
              markDirty();                             // 过期态编辑：禁裸 PUT
              await new Promise((r) => setTimeout(r, 600));  // 越过防抖窗
              if (putCalls !== put1)
                throw new Error(`过期态仍发 PUT（${put1}→${putCalls}）= 幻影编辑未堵住`);
              if (!/未同步/.test(saveHint.value))
                throw new Error("本地未同步提示缺失: " + saveHint.value);
              window.confirm = () => true;             // 横幅按钮 confirm 放行
              const btn = [...document.querySelectorAll("#stale-banner button")]
                .find((b) => b.textContent.includes("刷新加载最新"));
              if (!btn) throw new Error("「刷新加载最新」按钮缺失");
              btn.click();
              await new Promise((r) => setTimeout(r, 400));
              if (docStale.value) throw new Error("重载后未退出过期态");
              if ((doc.value.name || "") !== "云端最新" || doc.value.rev !== 99)
                throw new Error("重载未取到云端最新内容");
              await nextTick();
              if (document.querySelector("#stale-banner"))
                throw new Error("重载后横幅未消失");
              document.title = "SELFTEST-OK stale-409 | 409→横幅→禁PUT→重载恢复";
            } finally {
              window.fetch = realFetch; window.confirm = realConfirm;
              cancelAutoSave();
            }
            return;
          }
          else if (caseName === "offline-edit") {
            // [R8-2] 断连期间编辑守卫：conn 红 → markDirty 不调度 PUT +
            // 提示"离线，操作未保存"；恢复 conn 后不再拦截。
            if (!doc.value) throw new Error("no doc");
            const wasOk = connOk.value;
            try {
              connOk.value = false;
              const before = offlineLost;
              markDirty();
              if (offlineLost !== before + 1) throw new Error("离线编辑未计数");
              if (!/离线，操作未保存/.test(saveHint.value))
                throw new Error("离线提示缺失: " + saveHint.value);
              if (saveTimer) throw new Error("离线编辑仍调度了 PUT");
              connOk.value = true;
              markDirty();
              if (!/修改中/.test(saveHint.value))
                throw new Error("恢复后编辑未回到正常保存流: " + saveHint.value);
              document.title = "SELFTEST-OK offline-edit | 离线禁存+提示→恢复正常";
            } finally {
              connOk.value = wasOk;
              cancelAutoSave();
            }
            return;
          }
          else if (caseName === "del-key") {
            // Delete 键删片段验证：选中首个片段 → 模拟 window keydown → 断言减一。
            // 无副作用：删前记引用，断言后塞回原位（曾污染真实工程）。
            const ti = doc.value.scene.tracks.findIndex((t) => t.clips.length);
            if (ti < 0) throw new Error("no clips");
            selectedClip.value = [ti, 0];
            const track = doc.value.scene.tracks[ti];
            const removed = track.clips[0];         // 引用快照（还原用）
            const before = track.clips.length;
            window.dispatchEvent(new KeyboardEvent("keydown",
              { key: "Delete", code: "Delete", bubbles: true }));
            const after = track.clips.length;
            if (after !== before - 1)
              throw new Error(`del-key 未删: ${before}→${after}`);
            if (selectedClip.value !== null)
              throw new Error("del-key 后选中未清空");
            track.clips.unshift(removed);           // 塞回原位还原
            selectedClip.value = null;
            rebuildVis(); cancelAutoSave(); flushSave();
            document.title = `SELFTEST-OK del-key | clips ${before}→${after} 已还原`;
            return;
          }
          else if (caseName === "wheel-zoom") {
            // 内容区滚轮=纯缩放验证：dispatch wheel → 断言视窗 span 缩小且
            // center scrollTop 不变（vis 原生会同时垂直滚动=用户报的 bug）。
            const center = document.querySelector("#vis-timeline .vis-panel.vis-center");
            if (!center) throw new Error("no center panel");
            const visEl2 = document.querySelector("#vis-timeline");
            const rect = center.getBoundingClientRect();
            const cx = rect.left + rect.width / 2, cy = rect.top + rect.height / 2;
            const w0 = visTimeline.getWindow();
            const span0 = w0.end.valueOf() - w0.start.valueOf();
            const st0 = center.scrollTop;
            let atVisEl = false, atCenter = false;   // 传播探针
            const p1 = () => { atVisEl = true; };
            const p2 = () => { atCenter = true; };
            visEl2.addEventListener("wheel", p1, { capture: true, once: true });
            center.addEventListener("wheel", p2, { capture: true, once: true });
            center.dispatchEvent(new WheelEvent("wheel", {
              deltaY: -120, clientX: cx, clientY: cy,
              bubbles: true, cancelable: true }));
            const w1 = visTimeline.getWindow();
            const span1 = w1.end.valueOf() - w1.start.valueOf();
            const st1 = center.scrollTop;
            if (!(span1 < span0 * 0.95))
              throw new Error(`未缩放: span ${span0}→${span1} | visEl=${atVisEl} center=${atCenter}`);
            if (st1 !== st0)
              throw new Error(`滚轮引了垂直滚动: scrollTop ${st0}→${st1}`);
            document.title = `SELFTEST-OK wheel-zoom | span ${Math.round(span0/1e6)}s→` +
              `${Math.round(span1/1e6)}s | scrollTop ${st0}→${st1}`;
            return;
          }
          else if (caseName.startsWith("open:")) {
            // 打开指定 id 的时间轴（截图/排障用；等 bootstrap 后切换）
            await openTimeline(caseName.slice(5), true);
            await nextTick();
            document.title = "OPENED " + caseName.slice(5) +
              " | tracks=" + (doc.value ? doc.value.scene.tracks.length : 0);
            return;
          }
          else if (caseName.startsWith("scroll:")) {
            // 垂直滚动验证：先打开指定时间轴，再找 vis 内可滚容器
            // （scrollHeight > clientHeight）验证 scrollTop 可用
            await openTimeline(caseName.slice(7), true);
            await nextTick();
            await new Promise((r) => setTimeout(r, 400));   // 等 vis 渲染
            const all = [...document.querySelectorAll("#vis-timeline *")]
              .map((el) => ({ el, sh: el.scrollHeight, ch: el.clientHeight }))
              .filter(({ sh, ch }) => sh > ch + 10);
            if (!all.length) throw new Error("no scrollable el");
            const info = all.slice(0, 4).map(({ el, sh, ch }) => {
              const cls = (el.className && String(el.className)) || el.tagName;
              return `${String(cls).slice(0, 50)} sh=${sh} ch=${ch}`;
            });
            const p0 = all[0];
            p0.el.scrollTop = 99999;
            await new Promise((r) => setTimeout(r, 120));
            const moved = p0.el.scrollTop;
            p0.el.scrollTop = 0;
            document.title = `SELFTEST-OK scroll | n=${all.length} | ${info.join(" ; ")} | moved=${moved}`;
            return;
          }
          else if (caseName === "shot-collapsed") {
            // 截图专用：双侧栏收起后保持状态（供 headless --screenshot 拍折叠态）
            ui.left = true; ui.right = true;
            await nextTick();
            document.title = "SHOT-COLLAPSED";
            return;
          }
          else if (caseName === "shot-settings") {
            // 截图专用：打开设置浮层并填入真实值（三分组 + 运行模式徽章）
            ui.settings = true;
            await nextTick();
            await loadSettings();
            await nextTick();
            document.title = "SHOT-SETTINGS";
            return;
          }
          else if (caseName === "settings") {
            // 设置页读写闭环：读 → 改 render_fps=30 → PUT → GET 回读 → 还原
            ui.settings = true;
            await nextTick();
            await loadSettings();
            const old = st.display.render_fps;
            st.display.render_fps = 30;
            await saveSettings();
            const back = await api("/api/v1/settings");
            if (Number(back.display.render_fps) !== 30)
              throw new Error("settings 落盘不符: " + back.display.render_fps);
            st.display.render_fps = old;       // 还原（避免污染后续冒烟）
            await saveSettings();
            document.title = `SELFTEST-OK settings | render_fps ${old}→30→回读→还原` +
              ` | mode=${status.value && status.value.mode}`;
            return;
          }
          else if (caseName === "mpcdi-btn") {
            // 顶栏"▣ 输出校准"启用态一致性：拉状态 → 断言按钮 class/文案
            // 与 status.enabled 严格对应（启用=绿底+「启用中」标；未启用=默认态）
            await refreshMpcdiStatus();
            await nextTick();
            const btn = document.querySelector("#btn-mpcdi");
            if (!btn) throw new Error("btn-mpcdi missing");
            const on = !!(mpcdi.status && mpcdi.status.enabled);
            const cls = btn.classList.contains("warp-on");
            const tag = !!btn.querySelector(".warp-tag");
            if (cls !== on || tag !== on)
              throw new Error(`顶栏启用态不符: enabled=${on} class=${cls} tag=${tag}`);
            document.title = `SELFTEST-OK mpcdi-btn | enabled=${on}` +
              ` | title 含提示=${/变形/.test(btn.title)}`;
            return;
          }
          else if (caseName === "restart-flow") {
            // 模式切换自动重启（全 mock 不碰真实进程/配置）：mock fetch 后
            // 改运行模式保存 → 断言 restart POST 恰被调 1 次、restarting
            // 复位、浮层自动关。真实重启链路由 L2 API 冒烟（2.3）覆盖。
            if (!status.value) throw new Error("no status");
            const realFetch = window.fetch;
            let restartCalled = 0;
            const okResp = (d) => new Response(
              JSON.stringify({ ok: true, data: d === undefined ? null : d }),
              { status: 200, headers: { "Content-Type": "application/json" } });
            window.fetch = async (path, opts) => {
              const p = String(path);
              if (p === "/api/v1/system/restart") { restartCalled++; return okResp(); }
              if (p === "/api/v1/settings") return okResp({});
              if (p === "/api/v1/status") return okResp(status.value);
              return realFetch(path, opts);
            };
            try {
              ui.settings = true;
              await nextTick();
              const foot = document.querySelector("#settings-card .set-foot");
              if (!foot) throw new Error("settings foot missing");
              if (![...foot.querySelectorAll("button")]
                  .some((b) => b.textContent.includes("重启 display")))
                throw new Error("restart button missing");
              const before = status.value.mode;
              st.node.mode = before === "standalone" ? "sync" : "standalone";
              await saveSettings();
              if (restartCalled !== 1)
                throw new Error(`restart POST 调用=${restartCalled}（应恰 1 次）`);
              if (restarting.value) throw new Error("restarting 未复位");
              if (ui.settings) throw new Error("重启成功后设置浮层未关闭");
              if (!/已重启/.test(saveHint.value)) throw new Error("成功提示缺失");
              st.node.mode = before;            // 还原表单（mock PUT 未落盘）
              document.title = `SELFTEST-OK restart-flow | 模式切换→restart POST×1→浮层自关→表单还原`;
            } finally {
              window.fetch = realFetch;         // 恒恢复（断言失败也不漏）
            }
            return;
          }
          else if (caseName === "mpcdi") {
            // MPCDI 独立页验证：打开校准页 → 拉状态/包列表 → 状态条徽章渲染
            // （2026-11-12 校准页从设置浮层独立——原查 settings fieldset 已过时）
            ui.mpcdi = true;
            await nextTick();
            await refreshMpcdi();
            await nextTick();
            const badge = document.querySelector("#mpcdi-card .m-status .chip");
            if (!badge) throw new Error("MPCDI status badge missing");
            const en = mpcdi.status && mpcdi.status.enabled;
            document.title = `SELFTEST-OK mpcdi | badge="${badge.textContent.trim()}"` +
              ` enabled=${en} packages=${mpcdi.packages.length}` +
              ` | mode=${status.value && status.value.mode}`;
            return;
          }
          else if (caseName === "rename-tl") {
            // 原地改名验证：start→input 出现→设值→Enter→server 落盘→还原。
            // 无副作用：改名断言后写回原名。
            const t = timelines.value[0];
            if (!t) throw new Error("no timeline");
            const old = t.name || t.id;
            startRenameTimeline(t);
            await nextTick();
            const input = document.querySelector("#tl-list input.inline-rename");
            if (!input) throw new Error("no inline rename input");
            if (document.activeElement !== input)
              throw new Error("rename input not focused");
            input.value = "SELFTEST-改名";
            input.dispatchEvent(new Event("input", { bubbles: true }));  // v-model
            input.dispatchEvent(new KeyboardEvent("keyup", { key: "Enter", bubbles: true }));
            await new Promise((r) => setTimeout(r, 700));   // 等自动保存 PUT
            const back = await api(`/api/v1/timelines/${enc(t.id)}`);
            if ((back.name || "") !== "SELFTEST-改名")
              throw new Error(`rename 落盘不符: "${back.name}"`);
            if (t.id === currentId.value && doc.value) {
              doc.value.name = old; markDirty();            // 还原
            } else {
              const d = await api(`/api/v1/timelines/${enc(t.id)}`);
              d.name = old;
              await api(`/api/v1/timelines/${enc(t.id)}`, { method: "PUT",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(d) });
            }
            await new Promise((r) => setTimeout(r, 700));
            // blur=取消语义（2026-11-12 定案）：草稿丢弃、原名保留
            startRenameTimeline(t);
            await nextTick();
            const inp2 = document.querySelector("#tl-list input.inline-rename");
            if (!inp2) throw new Error("second rename input missing");
            inp2.value = "未提交草稿";
            inp2.dispatchEvent(new Event("input", { bubbles: true }));
            inp2.blur();
            await nextTick();
            if (renamingTl.value !== null)
              throw new Error("blur 后编辑态未退出");
            if ((timelines.value[0].name || "") === "未提交草稿")
              throw new Error("blur 应取消改名（草稿被误提交）");
            document.title = `SELFTEST-OK rename-tl | "${old}"→改名落盘→已还原 | blur 取消 ✓`;
            return;
          }
          else if (caseName === "rename-track") {
            // 轨道名牌原地改名验证：模拟真实双击序列 click#1→click#2→dblclick。
            // 先选中另一条轨，强制 click#1 走"选中态变化→rebuildVis"重建路径
            // （=原 bug 根因场景：dblclick target 成游离节点，输入框无处挂载）。
            const track = doc.value.scene.tracks[0];
            if (!track) throw new Error("no track");
            const old = track.name || "轨 1";
            if (doc.value.scene.tracks.length > 1) selectedTrack.value = 1;
            const nm = () => document.querySelector('#vis-timeline .tname[data-ti="0"]');
            let nameEl = nm();
            if (!nameEl) throw new Error("no track label");
            const click = (el) => el.dispatchEvent(new MouseEvent("click",
              { bubbles: true, cancelable: true }));
            click(nameEl);                                   // 第 1 击：选中→重建
            nameEl = nm() || nameEl;                         // 重建后取新节点
            click(nameEl);                                   // 第 2 击
            nameEl.dispatchEvent(new MouseEvent("dblclick",
              { bubbles: true, cancelable: true }));
            const input = document.querySelector(
              "#vis-timeline input.inline-rename");
            if (!input || !input.isConnected)
              throw new Error("改名输入框未挂到文档（游离节点 bug 复现）");
            if (document.activeElement !== input)
              throw new Error("rename input not focused");
            input.value = "SELFTEST-轨名";
            input.dispatchEvent(new KeyboardEvent("keydown",
              { key: "Enter", bubbles: true }));
            await new Promise((r) => setTimeout(r, 700));    // 等自动保存 PUT
            const back = await api(`/api/v1/timelines/${enc(currentId.value)}`);
            if ((back.scene.tracks[0].name || "") !== "SELFTEST-轨名")
              throw new Error(`rename 落盘不符: "${back.scene.tracks[0].name}"`);
            doc.value.scene.tracks[0].name = old; markDirty();   // 还原
            await new Promise((r) => setTimeout(r, 700));
            document.title = `SELFTEST-OK rename-track | "${old}"→改名落盘→已还原`;
            return;
          }
          else if (caseName === "rename-blur") {
            // 改名中点击其他轨道（原 bug 时序）：双击轨 0 改名中 → mousedown
            // 到轨道 1 → input blur（finish）→ click 轨道 1（选中态 0→1 变化
            // → rebuildVis）。修复前 blur 里也 rebuildVis=连续两次 DataSet
            // clear+add，vis 异步重绘撞车 → duplicate id / hide 崩溃；
            // 修复后 finish 只就地恢复 span=全流程仅 click 一次重建。
            if (doc.value.scene.tracks.length < 2) {
              document.title = "SELFTEST-OK rename-blur | 单轨跳过";
              return;
            }
            const errs = [];
            const onErr = (e) => errs.push(e.message || String(e));
            window.addEventListener("error", onErr);
            try {
              selectedTrack.value = 0;
              rebuildVis();
              await new Promise((r) => setTimeout(r, 100));
              const nm = () => document.querySelector('#vis-timeline .tname[data-ti="0"]');
              let nameEl = nm();
              if (!nameEl) throw new Error("no track label");
              const click = (el) => el.dispatchEvent(new MouseEvent("click",
                { bubbles: true, cancelable: true }));
              click(nameEl);                       // 双击序列：click×2 + dblclick
              nameEl = document.querySelector('#vis-timeline .tname[data-ti="0"]') || nameEl;
              click(nameEl);
              nameEl.dispatchEvent(new MouseEvent("dblclick",
                { bubbles: true, cancelable: true }));
              const input = document.querySelector(
                "#vis-timeline input.inline-rename");
              if (!input || !input.isConnected)
                throw new Error("改名框未挂载");
              input.value = "未提交草稿";          // blur=取消语义：草稿必须丢弃
              // 真实点击时序：mousedown（焦点转移→blur=finish）→ mouseup → click
              const other = document.querySelector('#vis-timeline .tname[data-ti="1"]');
              if (!other) throw new Error("no other track label");
              other.dispatchEvent(new MouseEvent("mousedown",
                { bubbles: true, cancelable: true }));
              input.blur();                        // 焦点转移（合成事件不自动 blur）
              other.dispatchEvent(new MouseEvent("mouseup",
                { bubbles: true, cancelable: true }));
              click(other);                        // 选中轨道 1 → rebuildVis ×1
              await new Promise((r) => setTimeout(r, 400));   // vis 异步重绘窗口
              if (document.querySelector("#vis-timeline input.inline-rename"))
                throw new Error("改名框未就地恢复");
              if (doc.value.scene.tracks[0].name === "未提交草稿")
                throw new Error("blur 应取消改名（草稿被误提交）");
              const visErr = errs.filter((m) =>
                /duplicate|reading 'hide'|vis-timeline/i.test(m));
              if (visErr.length)
                throw new Error("vis 崩溃: " + visErr[0]);
              document.title = `SELFTEST-OK rename-blur | 改名中点击其他轨道无 vis 错误 | 选中→轨1`;
            } finally {
              window.removeEventListener("error", onErr);
              selectedTrack.value = 0;
              rebuildVis();
            }
            return;
          }
          else if (caseName === "line-probe") {
            // 分隔线诊断：量测首/末 group 与 label 的 border、盒高、位置
            const gs = [...document.querySelectorAll("#vis-timeline .vis-foreground > .vis-group")];
            const ls = [...document.querySelectorAll(".vis-labelset .vis-label")];
            if (!gs.length || !ls.length) throw new Error("no groups/labels");
            const info = (el) => {
              const cs = getComputedStyle(el);
              const r = el.getBoundingClientRect();
              return `h=${el.offsetHeight} bb=${cs.borderBottomWidth}/${cs.borderBottomColor} ` +
                     `top=${Math.round(r.top)} bot=${Math.round(r.bottom)}`;
            };
            document.title = `LINES | n=${gs.length}/${ls.length}` +
              ` | g0 ${info(gs[0])} | gN ${info(gs[gs.length - 1])}` +
              ` | l0 ${info(ls[0])} | lN ${info(ls[ls.length - 1])}`;
            return;
          }
          else { document.title = "SELFTEST-FAIL: unknown case " + caseName; return; }
          const after = doc.value.scene.tracks.map((t) => t.name).join(">");
          const labels = [...document.querySelectorAll("#vis-timeline .vis-labelset .vis-label .tname")]
            .map((el) => el.textContent).join(">");
          const visState = visTimeline
            ? `visRoot=${visTimeline.initialDrawDone ? "drawn" : "NOT-DRAWN"}/visible=${rootVisibility()}`
            : "vis=NULL";
          document.title = `SELFTEST-OK ${caseName} | before=${before} | after=${after} | labels=${labels} | ${visState}`;
        } catch (err) { document.title = "SELFTEST-FAIL: " + err.message; }
      }, 1500);
    }

    // ---- MPCDI 输出校准（M4 收编自 MMC；两模式均可用）------------------------
    // 单槽包存储（current）+ enable 本地构建顶点网格（服务端投递渲染线程）
    // + session 持久化（重启 display 自动恢复）。
    const mpcdi = reactive({
      status: null, packages: [], sel: "", regions: [],
      meta: null, layout: "grid", uploading: false, hint: "",
      // 拼接布局：auto=预设模式（mode 下拉）；custom=自定义摆放（mosaic——
      // 后端 layout.slots 行优先、空串=空格、同 region 可多格 + cols/rows）
      layoutKind: "auto",
      // slots 初始给满默认 2×2 空串——绑定为 undefined 时 select 无匹配
      // option 会显示空白（watch 只在列/行变化时同步，首进 custom 不触发）
      mosaicCols: 2, mosaicRows: 2, mosaicSlots: ["", "", "", ""],
      // flip 默认关（2026-11-11 实证：uv 直采即正向；flip 是物理装配
      // 补偿开关，按需勾选）
      opts: { alpha_linearize: false, flip_y: false, flip_screen_y: false,
              blend_enabled: true },
    });

    /** 输出校准启用中？（顶栏按钮启用态标识 + 启用/停用按钮状态切换） */
    const mpcdiWarpOn = computed(() => !!(mpcdi.status && mpcdi.status.enabled));

    /** 轻量刷新校准状态（只拉 status，不动包/region 选择）——启动预拉 +
     *  启用/停用后同步，顶栏"▣ 输出校准"按钮据此显示启用态。 */
    async function refreshMpcdiStatus() {
      try {
        const s = await api("/api/v1/mpcdi");
        mpcdi.status = s.data || s;
      } catch (e) { /* 状态获取失败不阻塞 UI（打开浮层时 refreshMpcdi 再试） */ }
    }

    /** 自定义摆放网格（cols×rows 展开为可绑定数组；不足补空串）。 */
    const mosaicCells = computed(() => {
      const n = Math.max(1, mpcdi.mosaicCols) * Math.max(1, mpcdi.mosaicRows);
      const out = [];
      for (let i = 0; i < n; ++i) out.push(mpcdi.mosaicSlots[i] || "");
      return out;
    });
    /** 尺寸变化时同步 slots 数组长度（保留已填内容）。 */
    watch(() => [mpcdi.mosaicCols, mpcdi.mosaicRows], () => {
      const n = Math.max(1, mpcdi.mosaicCols) * Math.max(1, mpcdi.mosaicRows);
      while (mpcdi.mosaicSlots.length < n) mpcdi.mosaicSlots.push("");
      mpcdi.mosaicSlots.length = n;
    });
    /** 按包内 region 顺序行优先填充网格（多余格留空）。
     *  [R8 改版] 参与 region 勾选步已删——参与集合=摆进格子的 region。 */
    function fillMosaicAuto() {
      const n = Math.max(1, mpcdi.mosaicCols) * Math.max(1, mpcdi.mosaicRows);
      const keys = mpcdi.regions.map((r) => r.key);
      mpcdi.mosaicSlots = keys.slice(0, n).concat(
        Array.from({ length: Math.max(0, n - keys.length) }, () => ""));
    }
    /** 自定义摆放的拼接输出分辨率预览（取所摆 region 的最大单格分辨率）。 */
    const mosaicRes = computed(() => {
      const used = new Set(mosaicCells.value.filter(Boolean));
      if (!used.size || !mpcdi.regions.length) return "";
      let w = 0, h = 0;
      for (const r of mpcdi.regions) {
        if (!used.has(r.key) && !used.has(r.id)) continue;
        w = Math.max(w, r.xResolution || 0); h = Math.max(h, r.yResolution || 0);
      }
      if (!w) return "";
      return `${w * Math.max(1, mpcdi.mosaicCols)}×${h * Math.max(1, mpcdi.mosaicRows)}`;
    });
    /** 启用按钮可用性：选了包 + 包内有 region +（自定义摆放时至少一格非空）。
     *  [R8 改版] 参与集合=布局摆入的 region（自动排列=包内全部）。 */
    const mpcdiCanEnable = computed(() =>
      !!mpcdi.sel && !!mpcdi.regions.length &&
      (mpcdi.layoutKind !== "custom" || mosaicCells.value.some(Boolean)));

    /** 拉状态 + 包列表；已启用时回填选项（session 侧真相）。 */
    async function refreshMpcdi() {
      try {
        const s = await api("/api/v1/mpcdi");
        mpcdi.status = s.data || s;
        const pk = await api("/api/v1/mpcdi/packages");
        mpcdi.packages = (pk.data && pk.data.packages) || (pk.packages) || [];
        if (mpcdi.status && mpcdi.status.enabled && mpcdi.status.package_id) {
          mpcdi.sel = mpcdi.status.package_id;
          await onMpcdiPackageChange();
        }
      } catch (e) { mpcdi.hint = "状态读取失败: " + e.message; }
    }

    /** 选中包变化 → region 目录 + metadata 面板（HTTP 失败静默清空）。
     *  [R8 改版] 目录只读展示（并入步骤 1）；参与集合由布局格子推导。 */
    async function onMpcdiPackageChange() {
      mpcdi.regions = []; mpcdi.meta = null;
      if (!mpcdi.sel) return;
      try {
        const rg = await api(`/api/v1/mpcdi/packages/${enc(mpcdi.sel)}/regions`);
        mpcdi.regions = (rg.data && rg.data.regions) || (rg.regions) || [];
        const md = await api(`/api/v1/mpcdi/packages/${enc(mpcdi.sel)}/metadata`);
        mpcdi.meta = md.data || md;
      } catch (e) { mpcdi.hint = "包解析失败: " + e.message; }
    }

    /** 上传（multipart 单文件；后端三段式落位为 current）。 */
    async function uploadMpcdi(ev) {
      const f = ev.target && ev.target.files && ev.target.files[0];
      ev.target.value = "";   // 允许重选同一文件
      if (!f) return;
      mpcdi.uploading = true; mpcdi.hint = "上传中…";
      try {
        const fd = new FormData();
        fd.append("file", f, f.name);
        const r = await api("/api/v1/mpcdi/upload", { method: "POST", body: fd });
        mpcdi.hint = "上传成功";
        mpcdi.sel = (r.data && r.data.package_id) || r.package_id || "current";
        await refreshMpcdi();
        await onMpcdiPackageChange();
      } catch (e) { mpcdi.hint = "上传失败: " + e.message; }
      mpcdi.uploading = false;
    }

    /** 启用变形融合（本地构建 + 投递渲染线程 + 会话落盘）。 */
    async function enableMpcdi() {
      mpcdi.hint = "构建中…（解析包 + PFM 降采样，大包需数秒）";
      try {
        // 自定义摆放=mosaic：slots 行优先（空串=空格）+ cols/rows；
        // 自动排列=预设 mode（grid/row/column/cols2~4）。
        // [R8 改版] 参与集合：自定义=slots 行优先去重（未摆入格子的
        // region 不参与；后端 mpcdi_service 对 slots 本就优先于 csv）；
        // 自动=包内全部 region（原勾选默认全选语义）。
        const layout = mpcdi.layoutKind === "custom"
          ? { mode: "grid", cols: mpcdi.mosaicCols, rows: mpcdi.mosaicRows,
              slots: [...mpcdi.mosaicSlots] }
          : { mode: mpcdi.layout };
        const partRegions = mpcdi.layoutKind === "custom"
          ? [...new Set(mpcdi.mosaicSlots.filter(Boolean))]
          : mpcdi.regions.map((r) => r.key);
        const body = {
          package_id: mpcdi.sel,
          regions: partRegions.join(","),
          layout,
          options: { ...mpcdi.opts },
        };
        const r = await api("/api/v1/mpcdi/enable", {
          method: "POST", headers: { "Content-Type": "application/json" },
          body: JSON.stringify(body),
        });
        mpcdi.status = r.data || r;
        mpcdi.hint = mpcdi.status && mpcdi.status.enabled
          ? "已启用——重启 display 后自动恢复"
          : "启用失败";
      } catch (e) { mpcdi.hint = "启用失败: " + e.message; }
    }

    /** 停用（撤几何 + 落盘关闭态；渲染端回落场景手动网格）。 */
    async function disableMpcdi() {
      try {
        const r = await api("/api/v1/mpcdi/disable", { method: "POST" });
        mpcdi.status = r.data || r;
        mpcdi.hint = "已停用";
      } catch (e) { mpcdi.hint = "停用失败: " + e.message; }
    }

    // 设置浮层打开时顺带刷新（首次进入即见状态；不阻塞设置页渲染）
    watch(() => ui.settings, (open) => {
      if (open) refreshMpcdi();
    });

    // ---- 模板绑定导出 ---------------------------------------------------------
    return {
      // 状态/派生
      timelines, currentId, doc, library, status, saveHint, playheadText,
      activeName, durationText, stateName, connText, connOk, ui, draft,
      selClipData, docStale, reloadStaleDoc,
      // 模式/设置
      st, settingsHint, restarting, modeIsStandalone, modeIsSync, screens,
      loadSettings, saveSettings, restartDisplay,
      // MPCDI 输出校准（M4）
      mpcdi, refreshMpcdi, refreshMpcdiStatus, mpcdiWarpOn, onMpcdiPackageChange,
      uploadMpcdi, enableMpcdi, disableMpcdi,
      mosaicCells, mosaicRes, fillMosaicAuto, mpcdiCanEnable,
      // 工具（模板用）
      fmtTime, fmtSize, enc, escapeHtml,
      // 动作
      newTimeline, doPlay, doStop, doPauseToggle, doPlayPause, setDefault, deleteTimeline, openTimeline,
      exportShow, importShow,
      toggleLoop,
      refreshLibrary, onLibMouseDown, appendClipFromLibrary, uploadFiles, addTextClip,
      addTrackUi, moveTrack, deleteTrack, startRenameTimeline, cancelRenameTimeline,
      commitRenameTimeline, renamingTl, renameDraft, delClip, onProp,
      fitAll, zoomIn, zoomOut, forceSave,
    };
  },
});

// 组件内错误兜底：Vue prod 吞掉 setup/render 异常（console.error 而已，
// 不触发 window.onerror）——没有这条，整个 #app 只会静默渲染成空注释。
app.config.errorHandler = (err, _inst, info) =>
  showFatal(`Vue: ${err && err.message ? err.message : err} @ ${info}`);
app.mount("#app");

/** vis 根节点可见性（selftest 排障用：首绘竞态验证） */
function rootVisibility() {
  const el = document.querySelector("#vis-timeline .vis-timeline");
  return el ? getComputedStyle(el).visibility : "no-el";
}
