<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '开关',
  factory: 'createEPSwitchWidget',
  defaultW: 6,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    checked: false,
    onColor: '#13ce66',
    offColor: '#cbd5e1',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  valueMapper(value) {
    return { checked: toBool(value) };
  },
  coercers: {
    checked(v) {
      return toBool(v);
    },
  },
};
</script>

<script setup lang="ts">
import { computed, ref, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';
import { useContainerSize } from '../useFitSize';

const s = useWidgetState<{
  commandId: string;
  checked: boolean;
  onColor: string;
  offColor: string;
  borderColor: string;
  borderStyle: string;
}>();

const rootRef = ref<HTMLElement | null>(null);
const { width, height } = useContainerSize(rootRef);

watch(
  () => s.checked,
  (nv) => {
    sendCommand(s.commandId || '/cmd/demo', nv ? '1' : '0');
  },
);

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

const trackSize = computed(() => {
  const cw = width.value;
  const ch = height.value;
  if (!cw || !ch) return { w: 0, h: 0 };
  return {
    w: Math.min(cw, ch * 1.8),
    h: Math.min(ch, cw / 1.8),
  };
});

// 轨道尽量填满容器，保持约 1.8:1 的开关比例
const trackStyle = computed(() => {
  const on = !!s.checked;
  const { w: tw, h: th } = trackSize.value;
  return {
    width: tw ? `${tw}px` : '100%',
    height: th ? `${th}px` : '56%',
    borderRadius: '999px',
    background: on ? s.onColor : s.offColor,
    display: 'flex',
    alignItems: 'center',
    justifyContent: on ? 'flex-end' : 'flex-start',
    padding: '8%',
    boxSizing: 'border-box' as const,
    cursor: 'pointer',
    flexShrink: 0,
    transition: 'background-color 0.15s ease',
    border: 'none',
  };
});

const knobStyle = computed(() => {
  // 内容区高度 = 轨道高度扣除上下各 8% padding
  const knob = trackSize.value.h ? trackSize.value.h * 0.84 : 0;
  return {
    height: knob ? `${knob}px` : '100%',
    width: knob ? `${knob}px` : '56%',
    borderRadius: '50%',
    background: '#ffffff',
    flexShrink: 0,
    boxShadow: '0 1px 2px rgba(0,0,0,0.15)',
  };
});

function toggle() {
  s.checked = !s.checked;
}
</script>

<template>
  <div ref="rootRef" :style="containerStyle">
    <button
      type="button"
      class="ns-switch"
      :style="trackStyle"
      :aria-checked="s.checked"
      role="switch"
      @click="toggle"
    >
      <span class="ns-switch-knob" :style="knobStyle" />
    </button>
  </div>
</template>

<style scoped>
.ns-switch {
  margin: 0;
  appearance: none;
  -webkit-appearance: none;
}
</style>
