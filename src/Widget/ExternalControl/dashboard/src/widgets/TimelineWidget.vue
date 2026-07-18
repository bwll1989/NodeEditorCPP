<script lang="ts">
import type { WidgetMeta } from '../widget-types';

interface TimelineItem {
  id: string;
  text: string;
}

function coerceItems(v: unknown, fallbackItems: TimelineItem[]): TimelineItem[] {
  if (Array.isArray(v)) {
    return v.map((it) => ({
      id: it && (it.id ?? it.commandId) !== undefined ? String(it.id ?? it.commandId) : '',
      text: it && (it.text ?? it.name) !== undefined ? String(it.text ?? it.name) : '',
    }));
  }
  if (typeof v === 'string') {
    try {
      const j = JSON.parse(v);
      if (Array.isArray(j)) return coerceItems(j, fallbackItems);
    } catch {}
  }
  return Array.isArray(fallbackItems) ? fallbackItems : [];
}

const defaultItems: TimelineItem[] = [
  { id: '/cmd/item1', text: 'item 1' },
  { id: '/cmd/item2', text: 'item 2' },
  { id: '/cmd/item3', text: 'item 3' },
];

export const widgetMeta: WidgetMeta = {
  type: '时间线',
  factory: 'createEPTimelineWidget',
  defaultW: 24,
  defaultH: 6,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    fps: 25,
    tickStepSec: 15,
    items: defaultItems,
    lineColor: '#e5e7eb',
    pointColor: '#2563eb',
    textColor: '#111827',
    labelBgColor: '#ffffff',
    labelBorderColor: '#e5e7eb',
    axisY: 70,
    labelMaxWidth: 160,
    lineWidth: 2,
    borderColor: '#e5e7eb',
    borderStyle: 'none',
    value: 0,
  },
  coercers: {
    fontSize: (v) => String(v ?? '14'),
    fps: (v) => Number(v),
    tickStepSec: (v) => Number(v),
    items: (v) => coerceItems(v, defaultItems),
    axisY: (v) => Number(v),
    labelMaxWidth: (v) => Number(v),
    lineWidth: (v) => Number(v),
    value: (v) => Number(v),
  },
  valueMapper(value) {
    return { value: Number(value) };
  },
};
</script>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, reactive, ref } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  value: number;
  fps: number;
  tickStepSec: number;
  items: TimelineItem[];
  lineColor: string;
  pointColor: string;
  textColor: string;
  labelBgColor: string;
  labelBorderColor: string;
  axisY: number;
  labelMaxWidth: number;
  lineWidth: number;
  borderColor: string;
  borderStyle: string;
}>();

const valuesById = reactive<Record<string, number>>({});
const canvasWidth = ref(0);
const dragging = ref<{ key: string; id: string; pointerId: number; startedAt: number } | null>(null);
const dragHint = ref<{ left: number; text: string } | null>(null);
const selectedId = ref<string | null>(null);
const canvasRef = ref<HTMLElement | null>(null);

let dragStartClientX = 0;
let dragStarted = false;
let dragLastSendAt = 0;
let dragLastSentFrames: number | null = null;
let tlRo: ResizeObserver | null = null;

const safeFps = computed(() => {
  const n = Number(s.fps);
  return Number.isFinite(n) && n > 0 ? n : 25;
});

const safeTickStep = computed(() => {
  const n = Number(s.tickStepSec);
  return Number.isFinite(n) && n > 0 ? n : 15;
});

const normalizedItems = computed(() => {
  const fps = safeFps.value;
  const arr = Array.isArray(s.items) ? s.items : [];
  const out: Array<{
    id: string;
    text: string;
    frames: number;
    sec: number;
    key: string;
  }> = [];
  arr.forEach((it, idx) => {
    const id = it && it.id !== undefined && it.id !== null ? String(it.id) : '';
    const text = it && it.text !== undefined && it.text !== null ? String(it.text) : '';
    if (!id && !text) return;
    const v = id ? valuesById[id] : 0;
    const frames = Math.max(0, Math.floor(Number(v) || 0));
    out.push({
      id,
      text,
      frames,
      sec: frames / fps,
      key: (id || 'item') + '_' + String(idx),
    });
  });
  return out;
});

const rangeSec = computed(() => {
  const step = safeTickStep.value;
  const fps = safeFps.value;
  const items = normalizedItems.value;
  const cursorFrames = Math.max(0, Math.floor(Number(s.value) || 0));
  const cursorSec = cursorFrames / fps;

  if (!items.length) {
    const start = Math.floor(cursorSec / step) * step;
    const end = start + step;
    return { start, end };
  }

  let min = items[0].sec;
  let max = items[0].sec;
  items.forEach((it) => {
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
});

const cursor = computed(() => {
  const fps = safeFps.value;
  const frames = Math.max(0, Math.floor(Number(s.value) || 0));
  const sec = frames / fps;
  const { start, end } = rangeSec.value;
  const span = Math.max(0, end - start);
  const left = span > 0 ? ((sec - start) / span) * 100 : 0;
  const clamped = Math.min(100, Math.max(0, left));
  return { frames, sec, left: clamped, label: formatHMSF(frames) };
});

const ticks = computed(() => {
  const step = safeTickStep.value;
  const { start, end } = rangeSec.value;
  const span = Math.max(0, end - start);
  const maxTicks = 300;
  const count = Math.min(maxTicks, Math.floor(span / step) + 1);
  const out: Array<{ key: string; left: number; label: string }> = [];
  for (let i = 0; i < count; i++) {
    const sec = start + i * step;
    const left = span > 0 ? ((sec - start) / span) * 100 : 0;
    out.push({ key: 't_' + String(sec), left, label: formatTick(sec) });
  }
  return out;
});

const nodes = computed(() => {
  const { start, end } = rangeSec.value;
  const span = Math.max(0, end - start);
  const list = normalizedItems.value;
  const canvasW = Math.max(0, Number(canvasWidth.value) || 0);
  const labelW = Math.max(40, Math.floor(Number(s.labelMaxWidth) || 160));
  const gap = 10;

  const tmp = list
    .map((it) => {
      const left = span > 0 ? ((it.sec - start) / span) * 100 : 0;
      const x = canvasW > 0 ? (left / 100) * canvasW : left;
      return { it, left, x };
    })
    .sort((a, b) => a.x - b.x);

  const laneRight: number[] = [];
  const laneByKey: Record<string, number> = {};
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
    const tc = formatHMSF(it.frames);
    return {
      key: it.key,
      left,
      lane: laneByKey[it.key] ?? 0,
      text: it.text,
      id: it.id,
      frames: it.frames,
      title: (it.text ? it.text + '  ' : '') + tc,
    };
  });
});

const rootStyle = computed(() => {
  const y = Number(s.axisY);
  const labelMax = Number(s.labelMaxWidth);
  const lw = Number(s.lineWidth);
  return {
    width: '100%',
    height: '100%',
    boxSizing: 'border-box',
    borderWidth: '1px',
    borderColor: s.borderColor,
    borderStyle: s.borderStyle,
    borderRadius: '6px',
    '--ep-tl-y': (Number.isFinite(y) ? y : 70) + '%',
    '--ep-tl-line-color': s.lineColor,
    '--ep-tl-point-color': s.pointColor,
    '--ep-tl-text-color': s.textColor,
    '--ep-tl-label-bg': s.labelBgColor,
    '--ep-tl-label-border': s.labelBorderColor,
    '--ep-tl-label-max': (Number.isFinite(labelMax) ? labelMax : 160) + 'px',
    '--ep-tl-line-w': (Number.isFinite(lw) ? lw : 2) + 'px',
  };
});

function pad2(n: number) {
  return String(Math.max(0, Math.floor(Number(n) || 0))).padStart(2, '0');
}

function formatTick(sec: number) {
  const sv = Math.max(0, Math.floor(Number(sec) || 0));
  const h = Math.floor(sv / 3600);
  const m = Math.floor((sv % 3600) / 60);
  const ss = sv % 60;
  if (h > 0) return pad2(h) + ':' + pad2(m) + ':' + pad2(ss);
  return pad2(m) + ':' + pad2(ss);
}

function formatHMSF(frames: number) {
  const fps = safeFps.value;
  const f = Math.max(0, Math.floor(Number(frames) || 0));
  const totalSec = Math.floor(f / fps);
  const ff = f - totalSec * fps;
  const h = Math.floor(totalSec / 3600);
  const m = Math.floor((totalSec % 3600) / 60);
  const ss = totalSec % 60;
  return pad2(h) + ':' + pad2(m) + ':' + pad2(ss) + ':' + pad2(ff);
}

function secToFrames(sec: number) {
  const fps = safeFps.value;
  const sv = Math.max(0, Number(sec) || 0);
  return Math.max(0, Math.floor(sv * fps));
}

function clientXToPos(clientX: number) {
  const el = canvasRef.value;
  if (!el?.getBoundingClientRect) return null;
  const rect = el.getBoundingClientRect();
  const x = (Number(clientX) || 0) - rect.left;
  const w = Math.max(1, rect.width || 1);
  const ratio = Math.min(1, Math.max(0, x / w));
  const { start, end } = rangeSec.value;
  const sec = start + ratio * Math.max(0, end - start);
  const frames = secToFrames(sec);
  return { ratio, left: ratio * 100, frames };
}

function selectNode(n: { id?: string }) {
  if (!n?.id) return;
  selectedId.value = String(n.id);
}

function clearSelection() {
  selectedId.value = null;
}

function onRootPointerDown(ev: PointerEvent) {
  try {
    const t = ev.target as HTMLElement | null;
    if (t?.closest?.('.ep-tl-name')) return;
  } catch {}
  clearSelection();
}

function onNameKeydown(n: { id?: string; frames?: number }, ev: KeyboardEvent) {
  if (!n?.id || !ev) return;
  const k = ev.key;
  if (k !== 'ArrowLeft' && k !== 'ArrowRight') return;
  ev.preventDefault();
  const id = String(n.id);
  const cur =
    valuesById[id] !== undefined ? Number(valuesById[id]) : Number(n.frames || 0);
  const step = ev.shiftKey ? 10 : 1;
  const next = Math.max(0, Math.floor((cur || 0) + (k === 'ArrowRight' ? step : -step)));
  valuesById[id] = next;
  sendCommand(id, String(next));
}

function stopDragging() {
  window.removeEventListener('pointermove', onDragMove);
  window.removeEventListener('pointerup', onDragEnd);
  dragging.value = null;
  dragHint.value = null;
  dragStarted = false;
  dragStartClientX = 0;
}

function startDragging(n: { key: string; id: string }, ev: PointerEvent) {
  if (!n?.id || !ev) return;
  selectNode(n);
  try {
    (ev.currentTarget as HTMLElement)?.focus?.({ preventScroll: true });
  } catch {}
  try {
    (ev.currentTarget as HTMLElement)?.setPointerCapture?.(ev.pointerId);
  } catch {}
  dragStartClientX = Number(ev.clientX) || 0;
  dragStarted = false;
  dragging.value = { key: n.key, id: String(n.id), pointerId: ev.pointerId, startedAt: Date.now() };
  dragHint.value = null;
  dragLastSendAt = 0;
  dragLastSentFrames = null;
  window.addEventListener('pointermove', onDragMove, { passive: false });
  window.addEventListener('pointerup', onDragEnd, { passive: false });
}

function onDragMove(ev: PointerEvent) {
  if (!dragging.value) return;
  if (ev.cancelable) ev.preventDefault();
  if (!dragStarted) {
    const dx = Math.abs((Number(ev.clientX) || 0) - dragStartClientX);
    if (dx < 3) return;
    dragStarted = true;
  }
  const pos = clientXToPos(ev.clientX);
  if (!pos) return;
  valuesById[dragging.value.id] = pos.frames;
  dragHint.value = { left: pos.left, text: formatHMSF(pos.frames) + '  (' + String(pos.frames) + ')' };
  const now = Date.now();
  if (now - dragLastSendAt < 120) return;
  if (dragLastSentFrames === pos.frames) return;
  dragLastSendAt = now;
  dragLastSentFrames = pos.frames;
  sendCommand(dragging.value.id, String(pos.frames));
}

function onDragEnd(ev: PointerEvent) {
  if (!dragging.value) return;
  if (ev.cancelable) ev.preventDefault();
  if (!dragStarted) {
    stopDragging();
    return;
  }
  const id = dragging.value.id;
  const pos = clientXToPos(ev.clientX);
  if (pos) {
    valuesById[id] = pos.frames;
    dragHint.value = { left: pos.left, text: formatHMSF(pos.frames) + '  (' + String(pos.frames) + ')' };
    sendCommand(id, String(pos.frames));
  }
  stopDragging();
}

function handleWsMessage(e: Event) {
  const msg = (e as CustomEvent).detail;
  if (!msg || msg.commandId === undefined || msg.value === undefined) return;
  const addr = String(msg.commandId);
  const nv = Math.max(0, Math.floor(Number(msg.value) || 0));
  const cursorAddr = String(s.commandId || '');
  if (cursorAddr && addr === cursorAddr) {
    s.value = nv;
  }
  const arr = Array.isArray(s.items) ? s.items : [];
  const hit = arr.some((it) => String(it?.id ?? '') === addr);
  if (!hit) return;
  valuesById[addr] = nv;
}

onMounted(() => {
  window.addEventListener('ws-message', handleWsMessage);
  const setW = () => {
    canvasWidth.value = canvasRef.value?.clientWidth ?? 0;
  };
  setW();
  if (typeof ResizeObserver !== 'undefined' && canvasRef.value) {
    tlRo = new ResizeObserver(() => setW());
    try {
      tlRo.observe(canvasRef.value);
    } catch {}
  }
});

onBeforeUnmount(() => {
  window.removeEventListener('ws-message', handleWsMessage);
  stopDragging();
  if (tlRo) {
    try {
      tlRo.disconnect();
    } catch {}
    tlRo = null;
  }
});
</script>

<template>
  <div class="ep-tl-root" :style="rootStyle" @pointerdown="onRootPointerDown">
    <div ref="canvasRef" class="ep-tl-canvas">
      <div class="ep-tl-base" />

      <div
        v-if="cursor && (s.commandId || '')"
        class="ep-tl-cursor"
        :style="{ left: (cursor.left ?? 0) + '%' }"
      >
        <div class="ep-tl-cursor-line" />
        <div class="ep-tl-cursor-label">{{ cursor.label }}</div>
      </div>

      <div class="ep-tl-ticks">
        <div
          v-for="t in ticks"
          :key="t.key"
          class="ep-tl-tick"
          :style="{ left: t.left + '%' }"
        >
          <div class="ep-tl-tick-line" />
          <div class="ep-tl-tick-label">{{ t.label }}</div>
        </div>
      </div>

      <div class="ep-tl-nodes">
        <div
          v-for="n in nodes"
          :key="n.key"
          class="ep-tl-node"
          :title="n.title"
          :style="{ left: n.left + '%', '--ep-tl-lane': n.lane ?? 0 }"
        >
          <div
            class="ep-tl-name"
            :class="{ 'is-selected': (selectedId || '') === (n.id || '') }"
            tabindex="0"
            @focus="selectNode(n)"
            @blur="clearSelection"
            @click.stop="selectNode(n)"
            @keydown.stop="onNameKeydown(n, $event)"
            @keydown.esc.stop.prevent="clearSelection"
            @pointerdown.stop="startDragging(n, $event)"
          >
            {{ n.text }}
          </div>
          <div class="ep-tl-leader" />
          <div class="ep-tl-dot" />
        </div>

        <div
          v-if="dragHint"
          class="ep-tl-hint"
          :style="{ left: (dragHint.left ?? 0) + '%' }"
        >
          {{ dragHint.text }}
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.ep-tl-root {
  position: relative;
  width: 100%;
  height: 100%;
  overflow: hidden;
  background: transparent;
}
.ep-tl-root .ep-tl-canvas {
  position: relative;
  height: 100%;
  min-width: 100%;
  width: 100%;
  box-sizing: border-box;
}
.ep-tl-root .ep-tl-base {
  position: absolute;
  left: 8px;
  right: 8px;
  top: var(--ep-tl-y, 70%);
  height: var(--ep-tl-line-w, 2px);
  background: var(--ep-tl-line-color, #e5e7eb);
  transform: translateY(-50%);
  pointer-events: none;
  z-index: 1;
}
.ep-tl-root .ep-tl-cursor {
  position: absolute;
  top: 0;
  bottom: 0;
  transform: translateX(-50%);
  pointer-events: none;
  z-index: 4;
}
.ep-tl-root .ep-tl-cursor-line {
  position: absolute;
  left: 50%;
  top: 0;
  bottom: 0;
  width: 1px;
  background: var(--ep-tl-point-color, #2563eb);
  opacity: 0.9;
  transform: translateX(-50%);
}
.ep-tl-root .ep-tl-cursor-label {
  position: absolute;
  left: 50%;
  top: calc(var(--ep-tl-y, 70%) - 12px);
  transform: translate(-50%, -100%);
  padding: 2px 6px;
  border-radius: 4px;
  background: rgba(17, 24, 39, 0.92);
  color: #ffffff;
  font-size: 12px;
  line-height: 1.4;
  white-space: nowrap;
}
.ep-tl-root .ep-tl-ticks,
.ep-tl-root .ep-tl-nodes {
  position: absolute;
  inset: 0;
  pointer-events: none;
}
.ep-tl-root .ep-tl-ticks {
  z-index: 2;
}
.ep-tl-root .ep-tl-nodes {
  z-index: 3;
}
.ep-tl-root .ep-tl-tick {
  position: absolute;
  top: 0;
  bottom: 0;
  height: 100%;
  transform: translateX(-50%);
  color: var(--ep-tl-text-color, #111827);
  font-size: 12px;
}
.ep-tl-root .ep-tl-tick-line {
  position: absolute;
  left: 50%;
  top: calc(var(--ep-tl-y, 70%) + 0px);
  width: 1px;
  height: 8px;
  background: var(--ep-tl-line-color, #e5e7eb);
  transform: translateX(-50%);
  opacity: 0.9;
}
.ep-tl-root .ep-tl-tick-label {
  position: absolute;
  left: 50%;
  top: calc(var(--ep-tl-y, 70%) + 10px);
  transform: translateX(-50%);
  white-space: nowrap;
  opacity: 0.85;
}
.ep-tl-root .ep-tl-node {
  position: absolute;
  top: 0;
  height: 100%;
  transform: translateX(-50%);
  width: var(--ep-tl-label-max, 160px);
  max-width: var(--ep-tl-label-max, 160px);
  pointer-events: auto;
  cursor: default;
}
.ep-tl-root .ep-tl-name {
  position: absolute;
  left: 50%;
  cursor: grab;
  touch-action: none;
  top: calc(8px + (var(--ep-tl-lane, 0) * 22px));
  transform: translateX(-50%);
  user-select: none;
  max-width: 100%;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  padding: 2px 6px;
  border: 1px solid var(--ep-tl-label-border, #e5e7eb);
  border-radius: 4px;
  background: transparent;
  color: var(--ep-tl-text-color, #111827);
  font-size: inherit;
  line-height: 1.4;
  box-sizing: border-box;
}
.ep-tl-root .ep-tl-name:focus,
.ep-tl-root .ep-tl-name.is-selected {
  outline: none;
  box-shadow: 0 0 0 2px var(--ep-tl-point-color, #2563eb);
}
.ep-tl-root .ep-tl-hint {
  position: absolute;
  top: calc(var(--ep-tl-y, 70%) - 10px);
  transform: translate(-50%, -100%);
  padding: 2px 6px;
  border-radius: 4px;
  background: rgba(17, 24, 39, 0.92);
  color: #ffffff;
  font-size: 12px;
  line-height: 1.4;
  pointer-events: none;
  z-index: 10;
  white-space: nowrap;
}
.ep-tl-root .ep-tl-leader {
  position: absolute;
  left: 50%;
  top: calc(34px + (var(--ep-tl-lane, 0) * 22px));
  width: 1px;
  height: calc(var(--ep-tl-y, 70%) - (42px + (var(--ep-tl-lane, 0) * 22px)));
  min-height: 10px;
  background: var(--ep-tl-line-color, #e5e7eb);
  transform: translateX(-50%);
  box-sizing: border-box;
}
.ep-tl-root .ep-tl-dot {
  position: absolute;
  left: 50%;
  top: var(--ep-tl-y, 70%);
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: var(--ep-tl-point-color, #2563eb);
  border: 2px solid transparent;
  transform: translate(-50%, -50%);
  box-sizing: border-box;
}
</style>
