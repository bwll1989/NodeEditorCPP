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
import { computed, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  checked: boolean;
  onColor: string;
  offColor: string;
  borderColor: string;
  borderStyle: string;
}>();

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
  containerType: 'size' as const,
}));

// 轨道尽量填满容器，保持约 1.8:1 的开关比例
const trackStyle = computed(() => {
  const on = !!s.checked;
  return {
    width: 'min(100cqw, calc(100cqh * 1.8))',
    height: 'min(100cqh, calc(100cqw / 1.8))',
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

const knobStyle = {
  height: '100%',
  aspectRatio: '1',
  borderRadius: '50%',
  background: '#ffffff',
  flexShrink: 0,
  boxShadow: '0 1px 2px rgba(0,0,0,0.15)',
};

function toggle() {
  s.checked = !s.checked;
}
</script>

<template>
  <div :style="containerStyle">
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
