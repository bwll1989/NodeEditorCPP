<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: 'Trigger 按钮',
  factory: 'createEPButtonWidget',
  defaultW: 8,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    label: '执行',
    buttonType: 'primary',
    plain: false,
    round: false,
    buttonColor: '',
    activeColor: '',
    textColor: '',
    borderColor: '',
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
  buttonType: string;
  plain: boolean;
  round: boolean;
  buttonColor: string;
  activeColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
  isActive: boolean;
}>();

const epType = computed(() => {
  const t = String(s.buttonType || 'primary').toLowerCase();
  if (['primary', 'success', 'warning', 'danger', 'info', 'default'].includes(t)) return t;
  return 'primary';
});

const buttonStyle = computed(() => {
  const style: Record<string, string> = {
    width: '100%',
    height: '100%',
    fontSize: (s.fontSize || '14') + 'px',
  };
  if (s.borderStyle && s.borderStyle !== 'none') {
    style.borderStyle = s.borderStyle;
  }
  const bg = s.isActive && s.activeColor ? s.activeColor : s.buttonColor;
  if (bg) {
    style['--el-button-bg-color'] = bg;
    style['--el-button-hover-bg-color'] = bg;
    style['--el-button-active-bg-color'] = s.activeColor || bg;
  }
  if (s.textColor) style['--el-button-text-color'] = s.textColor;
  if (s.borderColor) style['--el-button-border-color'] = s.borderColor;
  return style;
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
    class="ns-ep-btn"
    :type="epType === 'default' ? undefined : (epType as any)"
    :plain="!!s.plain"
    :round="!!s.round"
    :style="buttonStyle"
    @click="exec"
    @mousedown="onDown"
    @mouseup="onUp"
    @mouseleave="onUp"
    @touchstart.passive="onDown"
    @touchend="onUp"
  >
    {{ s.label }}
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
