/**
 * 构建入口：扫描 widgets/*Widget.vue 并注册到 EPWidgets。
 * 新增控件只需添加一个 .vue 文件（导出 widgetMeta），无需单独的 entry/meta 文件。
 */
import { registerSfcWidget } from '../register-sfc-widget';
import type { WidgetModule } from '../widget-types';

const modules = import.meta.glob<WidgetModule>('./*Widget.vue', { eager: true });

for (const mod of Object.values(modules)) {
  if (!mod?.default || !mod.widgetMeta) {
    continue;
  }
  registerSfcWidget(mod.default, mod.widgetMeta);
}
