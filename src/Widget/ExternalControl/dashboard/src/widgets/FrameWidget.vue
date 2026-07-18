<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: 'Frame',
  factory: 'createEPFrameWidget',
  defaultW: 28,
  defaultH: 16,
  defaults: {
    bgColor: 'transparent',
    borderColor: '#e5e7eb',
    borderStyle: 'solid',
    radius: 10,
    shadow: 'none',
    label: '',
    labelColor: '#111827',
    labelBgColor: '#f8fafc',
    labelFontSize: 14,
    labelFontWeight: 500,
    labelOffsetX: 12,
    labelOffsetY: 0,
  },
  coercers: {
    radius(v) {
      const n = Number(v);
      if (!Number.isFinite(n)) return 10;
      return Math.max(0, Math.min(60, Math.round(n)));
    },
    shadow(v) {
      return String(v ?? 'none');
    },
    labelFontSize(v) {
      const n = Number(v);
      if (!Number.isFinite(n)) return 14;
      return Math.max(10, Math.min(64, Math.round(n)));
    },
    labelOffsetX(v) {
      const n = Number(v);
      if (!Number.isFinite(n)) return 12;
      return Math.max(-2000, Math.min(2000, Math.round(n)));
    },
    labelOffsetY(v) {
      const n = Number(v);
      if (!Number.isFinite(n)) return 0;
      return Math.max(-2000, Math.min(2000, Math.round(n)));
    },
    labelFontWeight(v) {
      const s0 = String(v ?? 500).trim();
      const n = Number(s0);
      const s = Number.isFinite(n) ? String(Math.round(n)) : s0;
      const ok = new Set(['300', '400', '500', '600', '700', '800', '900']);
      return ok.has(s) ? s : '500';
    },
  },
  onNodeCreated(node) {
    try {
      node.classList.add('ns-frame');
    } catch {}
    try {
      node.style.zIndex = '0';
    } catch {}
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  bgColor: string;
  borderColor: string;
  borderStyle: string;
  radius: number;
  shadow: string;
  label: string;
  labelColor: string;
  labelBgColor: string;
  labelFontSize: number;
  labelFontWeight: string | number;
  labelOffsetX: number;
  labelOffsetY: number;
}>();

const rootStyle = computed(() => ({
  width: '100%',
  height: '100%',
  position: 'relative',
  overflow: 'visible',
}));

const frameStyle = computed(() => {
  const r = Number(s.radius);
  const rr = Number.isFinite(r) ? r : 10;
  const sh = String(s.shadow || '').trim() || 'none';
  return {
    width: '100%',
    height: '100%',
    boxSizing: 'border-box',
    border: `1px ${s.borderStyle} ${s.borderColor}`,
    borderRadius: rr + 'px',
    backgroundColor: s.bgColor,
    boxShadow: sh,
    overflow: 'hidden',
  };
});

const labelStyle = computed(() => {
  const text = s.label !== undefined && s.label !== null ? String(s.label).trim() : '';
  if (!text) return { display: 'none' };
  const fs0 = Number(s.labelFontSize);
  const fs = Number.isFinite(fs0) ? Math.max(10, Math.min(64, Math.round(fs0))) : 14;
  const fw0 = String(s.labelFontWeight ?? 500).trim();
  const fw = fw0 ? fw0 : '500';
  const ox0 = Number(s.labelOffsetX);
  const oy0 = Number(s.labelOffsetY);
  const ox = Number.isFinite(ox0) ? Math.max(-2000, Math.min(2000, Math.round(ox0))) : 12;
  const oy = Number.isFinite(oy0) ? Math.max(-2000, Math.min(2000, Math.round(oy0))) : 0;
  const bg = String(s.labelBgColor || '#f8fafc').trim() || '#f8fafc';
  const color = String(s.labelColor || '#111827').trim() || '#111827';
  return {
    position: 'absolute',
    top: '0px',
    left: '0px',
    transform: `translate(${ox}px, ${oy}px) translateY(-50%)`,
    fontSize: fs + 'px',
    fontWeight: fw,
    color,
    backgroundColor: bg,
    padding: '2px 8px',
    borderRadius: '6px',
    lineHeight: '1.2',
    whiteSpace: 'nowrap',
    pointerEvents: 'none',
  };
});
</script>

<template>
  <div class="ep-frame-root" :style="rootStyle">
    <div class="ep-frame-box" :style="frameStyle" />
    <div v-if="s.label" class="ep-frame-label" :style="labelStyle">{{ s.label }}</div>
  </div>
</template>

<style scoped>
.ep-frame-root {
  width: 100%;
  height: 100%;
  position: relative;
  overflow: visible;
}
.ep-frame-box {
  width: 100%;
  height: 100%;
}
.ep-frame-label {
  user-select: none;
}
</style>
