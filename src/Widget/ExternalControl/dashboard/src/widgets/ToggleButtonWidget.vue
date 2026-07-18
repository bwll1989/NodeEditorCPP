<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '切换按钮',
  factory: 'createEPToggleButtonWidget',
  defaultW: 8,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    labelOn: '开启',
    labelOff: '关闭',
    active: false,
    buttonColor: '#409EFF',
    activeColor: '#0e5d45',
    pressColor: '#0e5d45',
    textColor: '#ffffff',
    borderColor: '#409EFF',
    borderStyle: 'none',
  },
  valueMapper(value) {
    return { active: toBool(value) };
  },
};
</script>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  labelOn: string;
  labelOff: string;
  active: boolean;
  buttonColor: string;
  activeColor: string;
  pressColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
}>();

const isPressed = ref(false);

const buttonStyle = computed(() => {
  const bg = isPressed.value ? s.pressColor : s.active ? s.activeColor : s.buttonColor;
  return {
    width: '100%',
    height: '100%',
    '--el-button-bg-color': bg,
    '--el-button-hover-bg-color': bg,
    '--el-button-active-bg-color': s.pressColor,
    '--el-button-text-color': s.textColor,
    '--el-button-border-color': s.borderColor,
    borderStyle: s.borderStyle,
    fontSize: s.fontSize + 'px',
  };
});

function toggle() {
  s.active = !s.active;
  sendCommand(s.commandId || '/cmd/demo', s.active ? '1' : '0');
}

function onDown() {
  isPressed.value = true;
}

function onUp() {
  isPressed.value = false;
}
</script>

<template>
  <el-button
    type="primary"
    :style="buttonStyle"
    @click="toggle"
    @mousedown="onDown"
    @mouseup="onUp"
    @mouseleave="onUp"
    @touchstart="onDown"
    @touchend="onUp"
  >
    {{ s.active ? s.labelOn : s.labelOff }}
  </el-button>
</template>
