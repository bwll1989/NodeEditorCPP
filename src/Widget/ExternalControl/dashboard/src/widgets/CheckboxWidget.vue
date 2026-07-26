<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '勾选',
  factory: 'createEPCheckboxWidget',
  defaultW: 4,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    checked: false,
    activeColor: '#409EFF',
    borderColor: '#94a3b8',
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
  activeColor: string;
  borderColor: string;
  borderStyle: string;
}>();

watch(
  () => s.checked,
  (nv) => {
    sendCommand(s.commandId || '/cmd/demo', nv ? 1 : 0);
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

const boxStyle = computed(() => {
  const on = !!s.checked;
  return {
    width: 'min(80cqw, 80cqh)',
    height: 'min(80cqw, 80cqh)',
    borderRadius: '12%',
    boxSizing: 'border-box' as const,
    border: `max(1px, 0.08em) solid ${on ? s.activeColor : s.borderColor}`,
    background: on ? s.activeColor : 'transparent',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    cursor: 'pointer',
    flexShrink: 0,
    fontSize: 'min(80cqw, 80cqh)',
  };
});

function toggle() {
  s.checked = !s.checked;
}
</script>

<template>
  <div :style="containerStyle">
    <button
      type="button"
      class="ns-check-box"
      :style="boxStyle"
      :aria-checked="s.checked"
      role="checkbox"
      @click="toggle"
    >
      <svg
        v-if="s.checked"
        viewBox="0 0 16 16"
        width="70%"
        height="70%"
        aria-hidden="true"
      >
        <path
          d="M3.5 8.2 L6.5 11.2 L12.5 4.8"
          fill="none"
          stroke="#fff"
          stroke-width="2.2"
          stroke-linecap="round"
          stroke-linejoin="round"
        />
      </svg>
    </button>
  </div>
</template>

<style scoped>
.ns-check-box {
  padding: 0;
  margin: 0;
  appearance: none;
  -webkit-appearance: none;
}
</style>
