<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '旋钮',
  factory: 'createEPKnobWidget',
  defaultW: 4,
  defaultH: 4,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    min: 0,
    max: 100,
    value: 0,
    step: 1,
    valueColor: '#3B82F6',
    rangeColor: '#dfe7ef',
    strokeWidth: 14,
    showValue: true,
    textColor: '#495057',
    borderColor: '#dfe7ef',
    borderStyle: 'none',
  },
  coercers: {
    fontSize: (v) => String(v ?? '14'),
    min: (v) => Number(v),
    max: (v) => Number(v),
    value: (v) => Number(v),
    step: (v) => Number(v),
    strokeWidth: (v) => Number(v),
    showValue: (v) => toBool(v),
  },
  valueMapper(value) {
    return { value: Number(value) };
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  min: number;
  max: number;
  value: number;
  step: number;
  valueColor: string;
  rangeColor: string;
  strokeWidth: number;
  showValue: boolean;
  textColor: string;
  borderColor: string;
  borderStyle: string;
}>();

const containerStyle = computed(() => ({
  width: '100%',
  height: '100%',
  display: 'flex',
  justifyContent: 'center',
  alignItems: 'center',
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
}));

function polarToCartesian(centerX: number, centerY: number, radius: number, angleInDegrees: number) {
  const angleInRadians = ((angleInDegrees - 90) * Math.PI) / 180.0;
  return {
    x: centerX + radius * Math.cos(angleInRadians),
    y: centerY + radius * Math.sin(angleInRadians),
  };
}

function describeArc(x: number, y: number, radius: number, startAngle: number, endAngle: number) {
  const start = polarToCartesian(x, y, radius, endAngle);
  const end = polarToCartesian(x, y, radius, startAngle);
  const largeArcFlag = endAngle - startAngle <= 180 ? '0' : '1';
  return ['M', start.x, start.y, 'A', radius, radius, 0, largeArcFlag, 0, end.x, end.y].join(' ');
}

function mapRange(x: number, inMin: number, inMax: number, outMin: number, outMax: number) {
  return ((x - inMin) * (outMax - outMin)) / (inMax - inMin) + outMin;
}

const rangePath = computed(() => describeArc(50, 50, 40, -145, 145));

const valuePath = computed(() => {
  const angle = mapRange(s.value, s.min, s.max, -145, 145);
  return describeArc(50, 50, 40, -145, angle);
});

const valueText = computed(() => Math.round(s.value));

function updateValue(event: MouseEvent | null, touch?: Touch) {
  const element = (event?.currentTarget as SVGSVGElement | null) ?? null;
  if (!element) return;
  const rect = element.getBoundingClientRect();
  const clientX = touch ? touch.clientX : (event?.clientX ?? 0);
  const clientY = touch ? touch.clientY : (event?.clientY ?? 0);
  const x = clientX - (rect.left + rect.width / 2);
  const y = clientY - (rect.top + rect.height / 2);
  let deg = (Math.atan2(y, x) * 180) / Math.PI + 90;
  if (deg > 180) deg -= 360;
  if (deg < -145) deg = -145;
  if (deg > 145) deg = 145;
  const val = mapRange(deg, -145, 145, s.min, s.max);
  const stepped = Math.round(val / s.step) * s.step;
  s.value = Math.min(s.max, Math.max(s.min, stepped));
  sendCommand(s.commandId || '/cmd/demo', String(s.value));
}

function onMouseDown(event: MouseEvent) {
  updateValue(event);
  const onMouseMove = (e: MouseEvent) => {
    e.preventDefault();
    updateValue(e);
  };
  const onMouseUp = () => {
    window.removeEventListener('mousemove', onMouseMove);
    window.removeEventListener('mouseup', onMouseUp);
  };
  window.addEventListener('mousemove', onMouseMove);
  window.addEventListener('mouseup', onMouseUp);
}

function onTouchStart(event: TouchEvent) {
  if (event.touches.length !== 1) return;
  updateValue(null, event.touches[0]);
  const onTouchMove = (e: TouchEvent) => {
    if (e.touches.length === 1) {
      e.preventDefault();
      updateValue(null, e.touches[0]);
    }
  };
  const onTouchEnd = () => {
    window.removeEventListener('touchmove', onTouchMove);
    window.removeEventListener('touchend', onTouchEnd);
  };
  window.addEventListener('touchmove', onTouchMove, { passive: false });
  window.addEventListener('touchend', onTouchEnd);
}
</script>

<template>
  <div class="p-knob p-component" :style="containerStyle">
    <svg
      viewBox="0 0 100 100"
      class="p-knob-svg"
      style="width: 100%; height: 100%; cursor: pointer; touch-action: none"
      @mousedown="onMouseDown"
      @touchstart="onTouchStart"
    >
      <path
        :d="rangePath"
        :stroke="s.rangeColor"
        :stroke-width="s.strokeWidth"
        fill="none"
        class="p-knob-range"
      />
      <path
        :d="valuePath"
        :stroke="s.valueColor"
        :stroke-width="s.strokeWidth"
        fill="none"
        class="p-knob-value"
      />
      <text
        v-if="s.showValue"
        x="50"
        y="57"
        text-anchor="middle"
        :fill="s.textColor"
        font-size="20"
        font-family="var(--font-family, inherit)"
        font-weight="600"
        class="p-knob-text"
      >
        {{ valueText }}
      </text>
    </svg>
  </div>
</template>
