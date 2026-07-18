<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

type Point3 = [number, number, number];

/** 取数值；缺省 / 非有限 → fallback（默认 0） */
function numOr(v: unknown, fallback = 0): number {
  if (v === undefined || v === null || v === '') return fallback;
  const n = Number(v);
  return Number.isFinite(n) ? n : fallback;
}

function toPoint(v: unknown): Point3 | null {
  if (Array.isArray(v) && v.length >= 1) {
    const x = numOr(v[0]);
    const y = numOr(v[1]);
    const z = numOr(v[2]);
    // 至少提供了一个有效分量才算点
    if (v.slice(0, 3).some((c) => c !== undefined && c !== null && c !== '' && Number.isFinite(Number(c)))) {
      return [x, y, z];
    }
    return null;
  }
  if (v && typeof v === 'object') {
    const o = v as Record<string, unknown>;
    // VariableData / VecData：优先 default / values 数组
    const packed = o.default ?? o.values;
    if (Array.isArray(packed) && packed.length >= 1) {
      return toPoint(packed);
    }
    const hasAny =
      o.x !== undefined || o.X !== undefined ||
      o.y !== undefined || o.Y !== undefined ||
      o.z !== undefined || o.Z !== undefined;
    if (!hasAny) return null;
    return [numOr(o.x ?? o.X), numOr(o.y ?? o.Y), numOr(o.z ?? o.Z)];
  }
  return null;
}

function coercePoints(v: unknown): Point3[] {
  if (Array.isArray(v)) {
    return v.map(toPoint).filter((p): p is Point3 => !!p);
  }
  if (typeof v === 'string') {
    const s = v.trim();
    if (!s) return [];
    try {
      return coercePoints(JSON.parse(s));
    } catch {
      return [];
    }
  }
  return [];
}

export const widgetMeta: WidgetMeta = {
  type: '3D散点',
  factory: 'createEPScatter3DWidget',
  defaultW: 20,
  defaultH: 14,
  defaults: {
    commandId: '/cmd/scatter3d',
    bgColor: '#0f172a',
    fontSize: '14',
    points: [],
    incoming: null,
    maxPoints: 5000,
    symbolSize: 8,
    pointColor: '#38bdf8',
    borderColor: '#334155',
    borderStyle: 'solid',
    xLabel: 'X',
    yLabel: 'Y',
    zLabel: 'Z',
    showAxes: true,
    autoRotate: false,
  },
  // points / incoming 为运行时缓存，不写入布局，刷新后清空
  propKeys: [
    'commandId',
    'bgColor',
    'fontSize',
    'maxPoints',
    'symbolSize',
    'pointColor',
    'borderColor',
    'borderStyle',
    'xLabel',
    'yLabel',
    'zLabel',
    'showAxes',
    'autoRotate',
  ],
  valueMapper(value) {
    return { incoming: value };
  },
  coercers: {
    points: (v) => coercePoints(v),
    maxPoints(v) {
      const n = Number(v);
      if (!Number.isFinite(n)) return 5000;
      return Math.max(10, Math.floor(n));
    },
    symbolSize(v) {
      const n = Number(v);
      if (!Number.isFinite(n)) return 8;
      return Math.max(1, Math.min(40, n));
    },
    showAxes(v) {
      return toBool(v);
    },
    autoRotate(v) {
      return toBool(v);
    },
  },
};
</script>

<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import * as echarts from 'echarts/core';
import { TooltipComponent } from 'echarts/components';
import { CanvasRenderer } from 'echarts/renderers';
import { Scatter3DChart } from 'echarts-gl/charts';
import { Grid3DComponent } from 'echarts-gl/components';
import { useWidgetState } from '../useWidgetState';

echarts.use([TooltipComponent, CanvasRenderer, Scatter3DChart, Grid3DComponent]);

type Point3 = [number, number, number];

const s = useWidgetState<{
  commandId: string;
  bgColor: string;
  points: Point3[];
  incoming: unknown;
  maxPoints: number;
  symbolSize: number;
  pointColor: string;
  borderColor: string;
  borderStyle: string;
  xLabel: string;
  yLabel: string;
  zLabel: string;
  showAxes: boolean;
  autoRotate: boolean;
}>();

// 页面刷新 / 布局重载时丢弃点缓存（含旧布局里已序列化的 points）
s.points = [];
s.incoming = null;

const rootRef = ref<HTMLElement | null>(null);
const chartRef = ref<HTMLElement | null>(null);
let chart: echarts.ECharts | null = null;
let ro: ResizeObserver | null = null;

type ViewPreset = 'xy' | 'xz' | 'yz' | 'free';
const VIEW_PRESETS: Record<ViewPreset, { alpha: number; beta: number; label: string }> = {
  xy: { alpha: 90, beta: 0, label: 'XY' },
  xz: { alpha: 0, beta: 0, label: 'XZ' },
  yz: { alpha: 0, beta: 90, label: 'YZ' },
  free: { alpha: 25, beta: 40, label: '3D' },
};
const viewPreset = ref<ViewPreset>('free');
const viewAlpha = ref(VIEW_PRESETS.free.alpha);
const viewBeta = ref(VIEW_PRESETS.free.beta);

function applyViewPreset(preset: ViewPreset) {
  const v = VIEW_PRESETS[preset];
  viewPreset.value = preset;
  viewAlpha.value = v.alpha;
  viewBeta.value = v.beta;
  if (!chart) {
    ensureChart();
    return;
  }
  try {
    chart.setOption({
      grid3D: {
        viewControl: {
          alpha: v.alpha,
          beta: v.beta,
          animation: true,
          animationDurationUpdate: 280,
        },
      },
    });
  } catch {}
}

/** 单击循环：XY → XZ → YZ → 3D */
function cycleView() {
  const order: ViewPreset[] = ['xy', 'xz', 'yz', 'free'];
  const i = order.indexOf(viewPreset.value);
  applyViewPreset(order[(i + 1) % order.length]);
}

function parsePoint(v: unknown): Point3 | null {
  return toPoint(v);
}

function parsePoints(v: unknown): Point3[] {
  if (Array.isArray(v)) {
    return v.map(parsePoint).filter((p): p is Point3 => !!p);
  }
  return [];
}

function trimPoints(pts: Point3[]): Point3[] {
  const max = Number(s.maxPoints);
  const limit = Number.isFinite(max) && max > 0 ? Math.floor(max) : 5000;
  return pts.length > limit ? pts.slice(-limit) : pts;
}

function applyIncoming(raw: unknown) {
  if (raw === undefined || raw === null) return;

  let payload: unknown = raw;
  if (typeof raw === 'string') {
    const text = raw.trim();
    if (!text) return;
    if (text.toLowerCase() === 'clear') {
      s.points = [];
      return;
    }
    try {
      payload = JSON.parse(text);
    } catch {
      const parts = text.split(/[,;\s]+/).filter((t) => t.length > 0);
      const one = parsePoint(parts);
      if (one) s.points = trimPoints([...(Array.isArray(s.points) ? s.points : []), one]);
      return;
    }
  }

  if (payload === 'clear') {
    s.points = [];
    return;
  }

  if (Array.isArray(payload)) {
    // [[x,y,z], ...] 整体替换；单点 [x,y] / [x,y,z] 追加（缺省补 0）
    if (
      payload.length >= 1 &&
      payload.length <= 4 &&
      payload.every((n) => typeof n === 'number' || typeof n === 'string')
    ) {
      const one = parsePoint(payload);
      if (one) s.points = trimPoints([...(Array.isArray(s.points) ? s.points : []), one]);
      return;
    }
    s.points = trimPoints(parsePoints(payload));
    return;
  }

  if (payload && typeof payload === 'object') {
    const o = payload as Record<string, unknown>;
    if (o.clear === true) {
      s.points = [];
      return;
    }
    if (Array.isArray(o.points)) {
      const pts = parsePoints(o.points);
      if (o.append === true || o.mode === 'append') {
        s.points = trimPoints([...(Array.isArray(s.points) ? s.points : []), ...pts]);
      } else {
        s.points = trimPoints(pts);
      }
      return;
    }
    // 兼容整包 { address, value: {x,y,z} }（正常路径 valueMapper 已拆出 value）
    if (o.value !== undefined && o.x === undefined && o.X === undefined) {
      applyIncoming(o.value);
      return;
    }
    const one = parsePoint(o);
    if (one) {
      s.points = trimPoints([...(Array.isArray(s.points) ? s.points : []), one]);
    }
  }
}

const shellStyle = computed(() => ({
  width: '100%',
  height: '100%',
  boxSizing: 'border-box' as const,
  border: `1px ${s.borderStyle || 'solid'} ${s.borderColor || '#334155'}`,
  backgroundColor: s.bgColor || '#0f172a',
  overflow: 'hidden',
  position: 'relative' as const,
}));

function buildOption() {
  const data = Array.isArray(s.points) ? s.points : [];
  const showAxes = s.showAxes !== false;
  return {
    backgroundColor: 'transparent',
    tooltip: {},
    grid3D: {
      boxWidth: 100,
      boxHeight: 100,
      boxDepth: 100,
      show: showAxes,
      axisLine: { show: showAxes },
      axisTick: { show: showAxes },
      axisLabel: { show: showAxes },
      splitLine: { show: showAxes },
      viewControl: {
        projection: 'perspective',
        autoRotate: !!s.autoRotate,
        autoRotateSpeed: 8,
        distance: 200,
        alpha: viewAlpha.value,
        beta: viewBeta.value,
      },
      light: {
        main: { intensity: 1.2 },
        ambient: { intensity: 0.35 },
      },
    },
    xAxis3D: { type: 'value', name: showAxes ? (s.xLabel || 'X') : '', show: showAxes },
    yAxis3D: { type: 'value', name: showAxes ? (s.yLabel || 'Y') : '', show: showAxes },
    zAxis3D: { type: 'value', name: showAxes ? (s.zLabel || 'Z') : '', show: showAxes },
    series: [
      {
        type: 'scatter3D',
        data,
        symbolSize: Number(s.symbolSize) || 8,
        itemStyle: {
          color: s.pointColor || '#38bdf8',
          opacity: 0.9,
        },
        emphasis: {
          itemStyle: { color: '#fbbf24' },
        },
      },
    ],
  };
}

function hasDomSize(el: HTMLElement | null | undefined): boolean {
  return !!el && el.clientWidth > 0 && el.clientHeight > 0;
}

function renderChart(full = false) {
  if (!chart) return;
  chart.setOption(buildOption(), { notMerge: full });
}

function ensureChart() {
  const el = chartRef.value;
  if (!el || !hasDomSize(el)) return false;
  if (!chart) {
    chart = echarts.init(el);
  }
  renderChart(true);
  return true;
}

function resizeChart() {
  // 布局尚未给出宽高时跳过，避免 ECharts “Dom has no width or height”
  if (!hasDomSize(chartRef.value) && !hasDomSize(rootRef.value)) return;
  if (!chart) {
    ensureChart();
    return;
  }
  try {
    chart.resize();
  } catch {}
}

onMounted(() => {
  nextTick(() => {
    ensureChart();
  });
  if (rootRef.value && typeof ResizeObserver !== 'undefined') {
    ro = new ResizeObserver(() => resizeChart());
    ro.observe(rootRef.value);
  }
});

onBeforeUnmount(() => {
  try {
    ro?.disconnect();
  } catch {}
  ro = null;
  try {
    chart?.dispose();
  } catch {}
  chart = null;
});

watch(
  () => s.incoming,
  (v) => {
    if (v === undefined || v === null || v === '') return;
    applyIncoming(v);
    s.incoming = null;
  },
);

watch(
  () => [s.points, s.symbolSize, s.pointColor, s.xLabel, s.yLabel, s.zLabel, s.showAxes, s.autoRotate, s.bgColor] as const,
  () => renderChart(false),
  { deep: true },
);
</script>

<template>
  <div ref="rootRef" class="ep-scatter3d-root" :style="shellStyle">
    <div ref="chartRef" class="ep-scatter3d-chart" />
    <div class="ep-scatter3d-views" @pointerdown.stop @mousedown.stop>
      <button
        type="button"
        class="ep-scatter3d-view-btn ep-scatter3d-view-cycle"
        title="循环切换视图 XY → XZ → YZ → 3D"
        @click="cycleView"
      >
        视图 {{ VIEW_PRESETS[viewPreset].label }}
      </button>
      <button
        v-for="key in (['xy', 'xz', 'yz', 'free'] as ViewPreset[])"
        :key="key"
        type="button"
        class="ep-scatter3d-view-btn"
        :class="{ active: viewPreset === key }"
        :title="`切换到 ${VIEW_PRESETS[key].label} 视角`"
        @click="applyViewPreset(key)"
      >
        {{ VIEW_PRESETS[key].label }}
      </button>
    </div>
    <div class="ep-scatter3d-meta">{{ (s.points && s.points.length) || 0 }} pts</div>
  </div>
</template>

<style scoped>
.ep-scatter3d-root {
  min-width: 0;
  min-height: 0;
}
.ep-scatter3d-chart {
  width: 100%;
  height: 100%;
}
.ep-scatter3d-views {
  position: absolute;
  left: 8px;
  top: 8px;
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  z-index: 2;
  pointer-events: auto;
}
.ep-scatter3d-view-btn {
  margin: 0;
  padding: 2px 8px;
  border: 1px solid rgba(148, 163, 184, 0.45);
  border-radius: 4px;
  background: rgba(15, 23, 42, 0.72);
  color: rgba(226, 232, 240, 0.9);
  font-size: 11px;
  line-height: 1.4;
  cursor: pointer;
  user-select: none;
}
.ep-scatter3d-view-btn:hover {
  border-color: rgba(56, 189, 248, 0.7);
  color: #fff;
}
.ep-scatter3d-view-btn.active {
  background: rgba(56, 189, 248, 0.28);
  border-color: rgba(56, 189, 248, 0.85);
  color: #fff;
}
.ep-scatter3d-view-cycle {
  font-weight: 600;
}
.ep-scatter3d-meta {
  position: absolute;
  right: 8px;
  bottom: 6px;
  color: rgba(226, 232, 240, 0.75);
  font-size: 11px;
  pointer-events: none;
  user-select: none;
}
</style>
