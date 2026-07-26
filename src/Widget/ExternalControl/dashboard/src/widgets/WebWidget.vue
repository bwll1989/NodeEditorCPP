<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: 'Web',
  factory: 'createEPWebWidget',
  defaultW: 16,
  defaultH: 10,
  defaults: {
    commandId: '/cmd/web',
    bgColor: 'transparent',
    url: 'https://example.com',
    allowScripts: true,
    allowForms: true,
    allowSameOrigin: true,
    borderColor: '#e5e7eb',
    borderStyle: 'solid',
  },
  // 远程推送可用 text/value 更新 URL
  valueMapper(value) {
    const s = String(value ?? '').trim();
    if (!s) return {};
    return { url: s };
  },
  coercers: {
    allowScripts(v) {
      return toBool(v);
    },
    allowForms(v) {
      return toBool(v);
    },
    allowSameOrigin(v) {
      return toBool(v);
    },
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  url: string;
  allowScripts: boolean;
  allowForms: boolean;
  allowSameOrigin: boolean;
  borderColor: string;
  borderStyle: string;
}>();

function normalizeUrl(raw: string): string {
  const t = String(raw || '').trim();
  if (!t) return '';
  if (/^https?:\/\//i.test(t) || /^about:/i.test(t) || /^data:/i.test(t)) return t;
  return 'https://' + t;
}

const src = computed(() => normalizeUrl(s.url));

const sandbox = computed(() => {
  const tokens: string[] = [];
  if (s.allowScripts) tokens.push('allow-scripts');
  if (s.allowForms) tokens.push('allow-forms');
  if (s.allowSameOrigin) tokens.push('allow-same-origin');
  tokens.push('allow-popups', 'allow-popups-to-escape-sandbox');
  return tokens.join(' ');
});

const containerStyle = computed(() => ({
  width: '100%',
  height: '100%',
  boxSizing: 'border-box' as const,
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  borderWidth: s.borderStyle && s.borderStyle !== 'none' ? '1px' : '0',
  overflow: 'hidden',
  background: '#fff',
}));
</script>

<template>
  <div class="ns-web-widget" :style="containerStyle">
    <iframe
      v-if="src"
      class="ns-web-frame"
      :src="src"
      :sandbox="sandbox"
      referrerpolicy="no-referrer"
      title="Web"
    />
    <div v-else class="ns-web-empty">请设置网页 URL</div>
  </div>
</template>

<style scoped>
.ns-web-frame {
  width: 100%;
  height: 100%;
  border: 0;
  display: block;
  background: #fff;
}
.ns-web-empty {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #94a3b8;
  font-size: 13px;
  background: #f8fafc;
}
</style>
