<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: '超链接',
  factory: 'createEPLinkWidget',
  defaultW: 8,
  defaultH: 2,
  defaults: {
    title: '超链接',
    commandId: '/cmd/demo',
    label: '打开链接',
    href: 'https://example.com',
    targetBlank: true,
    buttonType: 'primary',
    plain: false,
    round: false,
    buttonColor: '',
    activeColor: '',
    textColor: '',
    borderColor: '',
    borderStyle: 'none',
    fontSize: '14',
    bgColor: 'transparent',
  },
  valueMapper(value) {
    const str = String(value ?? '').trim();
    if (/^https?:\/\//i.test(str)) return { href: str };
    return { label: str };
  },
};
</script>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  label: string;
  href: string;
  targetBlank: boolean;
  buttonType: string;
  plain: boolean;
  round: boolean;
  buttonColor: string;
  activeColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
}>();

const isActive = ref(false);

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
  const bg = isActive.value && s.activeColor ? s.activeColor : s.buttonColor;
  if (bg) {
    style['--el-button-bg-color'] = bg;
    style['--el-button-hover-bg-color'] = bg;
    style['--el-button-active-bg-color'] = s.activeColor || bg;
  }
  if (s.textColor) style['--el-button-text-color'] = s.textColor;
  if (s.borderColor) style['--el-button-border-color'] = s.borderColor;
  return style;
});

function openLink() {
  try {
    window.open(s.href, s.targetBlank ? '_blank' : '_self', 'noopener');
  } catch {}
}

function onDown() {
  isActive.value = true;
}

function onUp() {
  isActive.value = false;
}
</script>

<template>
  <el-button
    class="ns-ep-btn"
    :type="epType === 'default' ? undefined : (epType as any)"
    :plain="!!s.plain"
    :round="!!s.round"
    :style="buttonStyle"
    @click="openLink"
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
