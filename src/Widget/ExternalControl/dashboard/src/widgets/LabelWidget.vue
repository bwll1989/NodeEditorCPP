<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: '标签',
  factory: 'createEPLabelWidget',
  defaultW: 8,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    text: '标签',
    textColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  valueMapper(value) {
    return { text: String(value ?? '') };
  },
};
</script>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  text: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
}>();

function handleWsMessage(e: Event) {
  const msg = (e as CustomEvent).detail;
  if (msg && msg.commandId === s.commandId) {
    if (msg.value !== undefined) s.text = String(msg.value);
    else if (msg.text !== undefined) s.text = String(msg.text);
  }
}

onMounted(() => {
  window.addEventListener('ws-message', handleWsMessage);
});

onBeforeUnmount(() => {
  window.removeEventListener('ws-message', handleWsMessage);
});

const containerStyle = computed(() => ({
  width: '100%',
  height: '100%',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
}));

const labelStyle = computed(() => ({
  color: s.textColor,
  fontSize: s.fontSize + 'px',
}));
</script>

<template>
  <div :style="containerStyle">
    <span :style="labelStyle">{{ s.text }}</span>
  </div>
</template>
