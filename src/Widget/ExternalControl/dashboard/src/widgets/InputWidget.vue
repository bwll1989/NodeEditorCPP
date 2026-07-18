<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: '输入框',
  factory: 'createEPInputWidget',
  defaultW: 12,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    placeholder: '请输入',
    value: '',
    textColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  valueMapper(value) {
    return { value: String(value ?? '') };
  },
};
</script>

<script setup lang="ts">
import { computed, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  value: string;
  placeholder: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  bgColor: string;
}>();

watch(
  () => s.value,
  (nv) => {
    sendCommand(s.commandId || '/cmd/demo', String(nv));
  },
);

const inputStyle = computed(() => ({
  width: '100%',
  height: '100%',
  boxSizing: 'border-box',
  color: s.textColor,
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  backgroundColor: s.bgColor,
  '--el-input-border-color': s.borderColor,
  '--el-input-hover-border-color': s.borderColor,
  '--el-input-focus-border-color': s.borderColor,
  '--el-fill-color-blank': s.bgColor,
  '--el-input-bg-color': s.bgColor,
  '--el-color-white': s.bgColor,
}));
</script>

<template>
  <el-input v-model="s.value" :placeholder="s.placeholder" :style="inputStyle" />
</template>
