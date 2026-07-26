<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: 'Toggle 按钮',
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
    buttonType: 'primary',
    activeType: 'success',
    plain: false,
    round: false,
    buttonColor: '',
    activeColor: '',
    pressColor: '',
    textColor: '',
    borderColor: '',
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
  buttonType: string;
  activeType: string;
  plain: boolean;
  round: boolean;
  buttonColor: string;
  activeColor: string;
  pressColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
}>();

const isPressed = ref(false);

function normalizeType(v: unknown, fallback: string) {
  const t = String(v || fallback).toLowerCase();
  if (['primary', 'success', 'warning', 'danger', 'info', 'default'].includes(t)) return t;
  return fallback;
}

const epType = computed(() =>
  normalizeType(s.active ? s.activeType : s.buttonType, s.active ? 'success' : 'primary')
);

const buttonStyle = computed(() => {
  const style: Record<string, string> = {
    width: '100%',
    height: '100%',
    fontSize: (s.fontSize || '14') + 'px',
  };
  if (s.borderStyle && s.borderStyle !== 'none') {
    style.borderStyle = s.borderStyle;
  }
  const bg = isPressed.value && s.pressColor
    ? s.pressColor
    : s.active && s.activeColor
      ? s.activeColor
      : s.buttonColor;
  if (bg) {
    style['--el-button-bg-color'] = bg;
    style['--el-button-hover-bg-color'] = bg;
    style['--el-button-active-bg-color'] = s.pressColor || bg;
  }
  if (s.textColor) style['--el-button-text-color'] = s.textColor;
  if (s.borderColor) style['--el-button-border-color'] = s.borderColor;
  return style;
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
    class="ns-ep-btn"
    :type="epType === 'default' ? undefined : (epType as any)"
    :plain="!!s.plain && !s.active"
    :round="!!s.round"
    :style="buttonStyle"
    @click="toggle"
    @mousedown="onDown"
    @mouseup="onUp"
    @mouseleave="onUp"
    @touchstart.passive="onDown"
    @touchend="onUp"
  >
    {{ s.active ? s.labelOn : s.labelOff }}
  </el-button>
</template>

<style scoped>
.ns-ep-btn {
  margin: 0;
  display: inline-flex;
  align-items: center;
  justify-content: center;
}
.ns-ep-btn :deep(span) {
  line-height: 1.2;
}
</style>
