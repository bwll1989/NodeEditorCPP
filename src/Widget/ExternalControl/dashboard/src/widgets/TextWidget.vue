<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

const TEXT_TYPES = new Set(['', 'primary', 'success', 'info', 'warning', 'danger']);

function normalizeType(v: unknown): string {
  const s = String(v ?? '').trim().toLowerCase();
  return TEXT_TYPES.has(s) ? s : '';
}

export const widgetMeta: WidgetMeta = {
  type: 'Text',
  factory: 'createEPTextWidget',
  defaultW: 12,
  defaultH: 4,
  defaults: {
    commandId: '/cmd/text',
    bgColor: 'transparent',
    fontSize: '14',
    text: 'Text',
    textType: '',
    truncated: false,
    lineClamp: 0,
    textColor: '',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
    align: 'left',
  },
  // 远程 value 更新显示文本（可含换行）
  valueMapper(value) {
    return { text: String(value ?? '') };
  },
  coercers: {
    textType(v) {
      return normalizeType(v);
    },
    truncated(v) {
      return toBool(v);
    },
    lineClamp(v) {
      const n = Math.floor(Number(v));
      return Number.isFinite(n) && n > 0 ? n : 0;
    },
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  text: string;
  textType: string;
  truncated: boolean;
  lineClamp: number;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  align: string;
  fontSize: string;
}>();

const containerStyle = computed(() => {
  const align = String(s.align || 'left');
  const justify =
    align === 'center'
      ? 'center'
      : align === 'right' || align === 'end'
        ? 'flex-end'
        : 'flex-start';
  return {
    width: '100%',
    height: '100%',
    display: 'flex',
    alignItems: 'flex-start',
    justifyContent: justify,
    boxSizing: 'border-box' as const,
    borderColor: s.borderColor,
    borderStyle: s.borderStyle,
    overflow: 'hidden',
    padding: '4px 6px',
  };
});

const textStyle = computed(() => {
  const style: Record<string, string> = {
    maxWidth: '100%',
    whiteSpace: s.truncated && !lineClamp.value ? 'nowrap' : 'pre-wrap',
    wordBreak: 'break-word',
    overflowWrap: 'anywhere',
    display: lineClamp.value ? '-webkit-box' : 'inline-block',
  };
  if (s.textColor) style.color = s.textColor;
  if (s.fontSize) style.fontSize = String(s.fontSize) + 'px';
  return style;
});

const epType = computed(() => normalizeType(s.textType) || undefined);
const lineClamp = computed(() => {
  const n = Math.floor(Number(s.lineClamp));
  return Number.isFinite(n) && n > 0 ? n : undefined;
});

// 多行内容按行拆分，保留空行
const lines = computed(() => String(s.text ?? '').split(/\r?\n/));
</script>

<template>
  <div class="ns-text-widget" :style="containerStyle">
    <el-text
      tag="div"
      :type="epType"
      :truncated="!!s.truncated && !lineClamp"
      :line-clamp="lineClamp"
      :style="textStyle"
    >
      <template v-for="(line, i) in lines" :key="i">
        <br v-if="i > 0" />
        <span>{{ line }}</span>
      </template>
    </el-text>
  </div>
</template>
