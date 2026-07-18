<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '按钮',
  factory: 'createEPButtonWidget',
  defaultW: 8,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    label: '执行',
    buttonColor: '#409EFF',
    activeColor: '#0e5d45',
    textColor: '#ffffff',
    borderColor: '#409EFF',
    borderStyle: 'none',
    isActive: false,
  },
  valueMapper(value) {
    return { isActive: toBool(value) };
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  label: string;
  buttonColor: string;
  activeColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
  isActive: boolean;
}>();

const buttonStyle = computed(() => {
  const bg = s.isActive ? s.activeColor : s.buttonColor;
  return {
    width: '100%',
    height: '100%',
    '--el-button-bg-color': bg,
    '--el-button-hover-bg-color': bg,
    '--el-button-active-bg-color': s.activeColor,
    '--el-button-text-color': s.textColor,
    '--el-button-border-color': s.borderColor,
    borderStyle: s.borderStyle,
    fontSize: s.fontSize + 'px',
  };
});

function exec() {
  sendCommand(s.commandId || '/cmd/demo', 1);
}

function onDown() {
  s.isActive = true;
}

function onUp() {
  s.isActive = false;
}
</script>

<template>
  <el-button
    type="primary"
    :style="buttonStyle"
    @click="exec"
    @mousedown="onDown"
    @mouseup="onUp"
    @mouseleave="onUp"
    @touchstart="onDown"
    @touchend="onUp"
  >
    {{ s.label }}
  </el-button>
</template>
