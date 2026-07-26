<script lang="ts">
import type { WidgetMeta } from '../widget-types';

function normalizeDirection(v: unknown): 'horizontal' | 'vertical' {
  const s = String(v ?? '').trim().toLowerCase();
  if (s === 'vertical' || s === 'v' || s === '纵向' || s === '竖向' || s === 'column') return 'vertical';
  return 'horizontal';
}

function normalizeBorderStyle(v: unknown): string {
  const s = String(v ?? '').trim().toLowerCase();
  if (['solid', 'dashed', 'dotted', 'double'].includes(s)) return s;
  return 'solid';
}

export const widgetMeta: WidgetMeta = {
  type: '分割线',
  factory: 'createEPDividerWidget',
  defaultW: 24,
  defaultH: 1,
  defaults: {
    bgColor: 'transparent',
    fontSize: '14',
    text: '',
    direction: 'horizontal',
    contentPosition: 'center',
    lineColor: '#dcdfe6',
    lineWidth: 1,
    borderStyle: 'solid',
    textColor: '#909399',
  },
  coercers: {
    direction(v) {
      return normalizeDirection(v);
    },
    lineWidth(v) {
      const n = Number(v);
      return Number.isFinite(n) ? Math.max(1, n) : 1;
    },
  },
};
</script>

<script setup lang="ts">
import { computed, toRef } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  text: string;
  direction: string;
  contentPosition: string;
  lineColor: string;
  lineWidth: number;
  borderStyle: string;
  textColor: string;
}>();

const directionRef = toRef(s, 'direction');
const isVertical = computed(() => normalizeDirection(directionRef.value) === 'vertical');
const showText = computed(() => String(s.text || '').trim().length > 0);

const contentPos = computed(() => {
  const p = String(s.contentPosition || 'center').toLowerCase();
  if (p === 'left' || p === 'right' || p === 'center') return p;
  return 'center';
});

const borderStyle = computed(() => normalizeBorderStyle(s.borderStyle));

const rootStyle = computed(() => {
  const w = Math.max(1, Number(s.lineWidth) || 1);
  return {
    width: '100%',
    height: '100%',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    boxSizing: 'border-box' as const,
    '--el-border-color': s.lineColor || '#dcdfe6',
    '--ns-divider-width': w + 'px',
    '--ns-divider-text': s.textColor || '#909399',
  };
});
</script>

<template>
  <div
    class="ns-divider-ep"
    :class="isVertical ? 'is-vertical' : 'is-horizontal'"
    :style="rootStyle"
  >
    <el-divider
      :direction="isVertical ? 'vertical' : 'horizontal'"
      :border-style="borderStyle"
      :content-position="contentPos"
    >
      <span v-if="showText" class="ns-divider-ep__text">{{ s.text }}</span>
    </el-divider>
  </div>
</template>

<style scoped>
.ns-divider-ep :deep(.el-divider) {
  margin: 0;
}
.ns-divider-ep.is-horizontal {
  flex-direction: row;
}
.ns-divider-ep.is-horizontal :deep(.el-divider--horizontal) {
  width: 100%;
  border-top-width: var(--ns-divider-width, 1px);
}
.ns-divider-ep.is-vertical {
  flex-direction: column;
}
.ns-divider-ep.is-vertical :deep(.el-divider--vertical) {
  height: 100%;
  border-left-width: var(--ns-divider-width, 1px);
}
.ns-divider-ep__text {
  color: var(--ns-divider-text, #909399);
  font-size: inherit;
  white-space: nowrap;
  user-select: none;
}
.ns-divider-ep.is-vertical .ns-divider-ep__text {
  writing-mode: vertical-rl;
  text-orientation: mixed;
  letter-spacing: 0.05em;
}
</style>
