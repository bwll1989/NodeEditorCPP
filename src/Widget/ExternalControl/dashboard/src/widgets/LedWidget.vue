<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: 'LED',
  factory: 'createEPLedWidget',
  defaultW: 4,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/status',
    bgColor: 'transparent',
    on: false,
    onColor: '#22c55e',
    offColor: '#cbd5e1',
    blink: false,
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  // 只读状态灯：WebSocket/HTTP 推送的 value 映射为 on，不向服务端发命令
  valueMapper(value) {
    return { on: toBool(value) };
  },
  coercers: {
    on(v) {
      return toBool(v);
    },
    blink(v) {
      return toBool(v);
    },
  },
};
</script>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { useWidgetState } from '../useWidgetState';
import { useContainerSize } from '../useFitSize';

const s = useWidgetState<{
  commandId: string;
  on: boolean;
  onColor: string;
  offColor: string;
  blink: boolean;
  borderColor: string;
  borderStyle: string;
}>();

const rootRef = ref<HTMLElement | null>(null);
const { width, height } = useContainerSize(rootRef);

const containerStyle = computed(() => ({
  width: '100%',
  height: '100%',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  boxSizing: 'border-box' as const,
}));

const ledStyle = computed(() => {
  const color = s.on ? s.onColor : s.offColor;
  const side = Math.max(0, Math.min(width.value, height.value) * 0.8);
  return {
    width: side ? `${side}px` : '80%',
    height: side ? `${side}px` : '80%',
    borderRadius: '50%',
    background: color,
    boxShadow: 'none',
    flexShrink: 0,
    animation: s.on && s.blink ? 'ns-led-blink 1s steps(1, end) infinite' : 'none',
  };
});
</script>

<template>
  <div
    ref="rootRef"
    class="ns-led-widget"
    :style="containerStyle"
    role="status"
    :aria-live="'polite'"
  >
    <span class="ns-led-dot" :style="ledStyle" :aria-label="s.on ? 'on' : 'off'" />
  </div>
</template>

<style>
@keyframes ns-led-blink {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.2;
  }
}
</style>
