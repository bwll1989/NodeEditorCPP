<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '勾选',
  factory: 'createEPCheckboxWidget',
  defaultW: 8,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    label: '启用',
    checked: false,
    activeColor: '#409EFF',
    textColor: '#111827',
    borderColor: '#409EFF',
    borderStyle: 'none',
  },
  valueMapper(value) {
    return { checked: toBool(value) };
  },
};
</script>

<script setup lang="ts">
import { computed, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  label: string;
  checked: boolean;
  activeColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
}>();

watch(
  () => s.checked,
  (nv) => {
    sendCommand(s.commandId || '/cmd/demo', nv ? 1 : 0);
  },
);

const checkboxStyle = computed(() => ({
  color: s.textColor,
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  '--el-checkbox-checked-bg-color': s.activeColor,
  '--el-checkbox-checked-border-color': s.borderColor,
  '--el-checkbox-text-color': s.textColor,
}));
</script>

<template>
  <div style="width:100%;height:100%;display:flex;align-items:center;justify-content:center;">
    <el-checkbox v-model="s.checked" :style="checkboxStyle">{{ s.label }}</el-checkbox>
  </div>
</template>
