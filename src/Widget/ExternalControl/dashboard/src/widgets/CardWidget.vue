<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: '卡片',
  factory: 'createEPCardWidget',
  defaultW: 28,
  defaultH: 16,
  defaults: {
    bgColor: '#ffffff',
    fontSize: '14',
    title: '',
    textColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'solid',
    radius: 10,
  },
  coercers: {
    fontSize: (v) => String(v ?? '14'),
    radius(v) {
      const n = Number(v);
      if (!Number.isFinite(n)) return 10;
      return Math.max(0, Math.min(30, Math.round(n)));
    },
  },
  onNodeCreated(node) {
    try {
      node.classList.add('ns-card');
    } catch {}
    try {
      node.style.zIndex = '0';
    } catch {}
    try {
      const cur = String((node.dataset && node.dataset.groupId) || '').trim();
      if (!cur) {
        const gid =
          'c_' + Math.random().toString(36).slice(2, 8) + Date.now().toString(36).slice(-4);
        node.dataset.groupId = gid;
      }
    } catch {}
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  bgColor: string;
  fontSize: string;
  title: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  radius: number;
}>();

const cardStyle = computed(() => {
  const r = Number(s.radius);
  const rr = Number.isFinite(r) ? r : 10;
  return {
    width: '100%',
    height: '100%',
    boxSizing: 'border-box',
    border: `1px ${s.borderStyle} ${s.borderColor}`,
    borderRadius: rr + 'px',
    backgroundColor: s.bgColor,
    overflow: 'hidden',
    display: 'flex',
    flexDirection: 'column',
  };
});

const titleStyle = computed(() => {
  const fs = Number(s.fontSize);
  const fontPx = Number.isFinite(fs) ? fs : 14;
  return {
    padding: '8px 10px',
    color: s.textColor,
    fontSize: fontPx + 'px',
    fontWeight: 600,
    borderBottom: '1px solid rgba(0,0,0,0.06)',
    background: 'transparent',
    userSelect: 'none',
    whiteSpace: 'nowrap',
    overflow: 'hidden',
    textOverflow: 'ellipsis',
  };
});
</script>

<template>
  <div class="ep-card-root" :style="cardStyle">
    <div v-if="(s.title || '').trim()" class="ep-card-title" :style="titleStyle">{{ s.title }}</div>
    <div class="ep-card-body" />
  </div>
</template>

<style scoped>
.ep-card-root {
  width: 100%;
  height: 100%;
}
.ep-card-body {
  flex: 1;
}
</style>
