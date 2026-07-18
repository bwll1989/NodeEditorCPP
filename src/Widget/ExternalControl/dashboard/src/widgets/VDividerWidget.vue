<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: '竖向分割线',
  factory: 'createEPVDividerWidget',
  defaultW: 1,
  defaultH: 10,
  defaults: {
    bgColor: 'transparent',
    fontSize: '14',
    lineColor: '#e5e7eb',
    lineWidth: 2,
    borderStyle: 'solid',
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  lineColor: string;
  lineWidth: number;
  borderStyle: string;
}>();

const dividerStyle = computed(() => ({
  height: '100%',
  borderLeft: `${s.lineWidth}px ${s.borderStyle} ${s.lineColor}`,
}));
</script>

<template>
  <div style="width:100%;height:100%;display:flex;justify-content:center;">
    <el-divider direction="vertical" :style="dividerStyle" />
  </div>
</template>
