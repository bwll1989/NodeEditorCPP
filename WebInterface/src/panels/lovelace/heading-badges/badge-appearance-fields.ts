import { html, type TemplateResult } from "lit";
import {
  BADGE_SIZE_PRESETS,
  badgeShowBackground,
  badgeShowText,
  badgeVisualSize,
  nearestBadgeSizePreset,
} from "./badge-flags";

export interface BadgeAppearanceConfig {
  show_name?: boolean;
  show_state?: boolean;
  show_background?: boolean;
  size?: number;
}

/** 图像 / 轨迹徽章共用的外观选项：文字、底色、大小。 */
export function renderBadgeAppearanceFields(
  config: BadgeAppearanceConfig,
  patch: (partial: Partial<BadgeAppearanceConfig>) => void,
  options?: {
    textLabel?: string;
    sizeFallback?: number;
  },
): TemplateResult {
  const textLabel = options?.textLabel ?? "显示文字";
  const size = badgeVisualSize(config, options?.sizeFallback);
  const preset = nearestBadgeSizePreset(size);

  return html`
    <label class="switch-row">
      <span>${textLabel}</span>
      <input
        type="checkbox"
        .checked=${badgeShowText(config)}
        @change=${(ev: Event) => {
          const checked = (ev.target as HTMLInputElement).checked;
          patch({ show_name: checked, show_state: checked });
        }}
      />
    </label>
    <label class="switch-row">
      <span>显示底色</span>
      <input
        type="checkbox"
        .checked=${badgeShowBackground(config)}
        @change=${(ev: Event) =>
          patch({ show_background: (ev.target as HTMLInputElement).checked })}
      />
    </label>
    <p class="helper">关闭底色后无胶囊填充；图标颜色随实体/按下状态变化。</p>
    <label class="field">
      <span>大小</span>
      <select
        @change=${(ev: Event) =>
          patch({ size: Number((ev.target as HTMLSelectElement).value) })}
      >
        ${BADGE_SIZE_PRESETS.map(
          (item) => html`
            <option value=${item.value} ?selected=${preset === item.value}>
              ${item.label}（高 ${item.value}px）
            </option>
          `,
        )}
      </select>
    </label>
  `;
}
