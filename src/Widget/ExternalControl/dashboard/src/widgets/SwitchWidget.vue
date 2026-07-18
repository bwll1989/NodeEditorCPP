<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '开关',
  factory: 'createEPSwitchWidget',
  defaultW: 8,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    checked: false,
    onColor: '#13ce66',
    offColor: '#ff4949',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  valueMapper(value) {
    return { checked: toBool(value) };
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
}));

const switchStyle = computed(() => ({
  '--el-switch-on-color': s.onColor,
  '--el-switch-off-color': s.offColor,
}));
</script>

<template>
  <div :style="containerStyle">
    <el-switch
      v-model="s.checked"
      :active-color="s.onColor"
      :inactive-color="s.offColor"
      :style="switchStyle"
    />
  </div>
</template>
