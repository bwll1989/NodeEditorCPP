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
    buttonColor: '#409EFF',
    activeColor: '#0e5d45',
    textColor: '#ffffff',
    borderColor: '#409EFF',
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
  buttonColor: string;
  activeColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
}>();

const isActive = ref(false);

const buttonStyle = computed(() => {
  const bg = isActive.value ? s.activeColor : s.buttonColor;
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
    type="primary"
    :style="buttonStyle"
    @click="openLink"
    @mousedown="onDown"
    @mouseup="onUp"
    @mouseleave="onUp"
    @touchstart="onDown"
    @touchend="onUp"
  >
    {{ s.label }}
  </el-button>
</template>
