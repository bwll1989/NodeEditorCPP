<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: '分割线',
  factory: 'createEPDividerWidget',
  defaultW: 24,
  defaultH: 1,
  defaults: {
    bgColor: 'transparent',
    fontSize: '14',
    text: '分割线',
    lineColor: '#e5e7eb',
    lineWidth: 2,
    borderStyle: 'solid',
    textColor: '#64748b',
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  text: string;
  lineColor: string;
  lineWidth: number;
  borderStyle: string;
  textColor: string;
}>();

const dividerStyle = computed(() => ({
  borderTop: `${s.lineWidth}px ${s.borderStyle} ${s.lineColor}`,
  color: s.textColor,
}));
</script>

<template>
  <div style="width:100%;height:100%;display:flex;align-items:center;justify-content:center;">
    <el-divider :style="dividerStyle">{{ s.text }}</el-divider>
  </div>
</template>
