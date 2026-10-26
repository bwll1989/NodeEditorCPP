/** Shared visibility / appearance flags for heading + picture / path badges. */

/** 与标题行控制徽章默认高度一致。 */
export const DEFAULT_BADGE_VISUAL_SIZE = 26;

/** 编辑器可选的徽章高度档位（横向胶囊）。 */
export const BADGE_SIZE_PRESETS = [
  { value: 22, label: "小" },
  { value: 26, label: "中" },
  { value: 32, label: "大" },
  { value: 40, label: "更大" },
] as const;

export function badgeShowIcon(config?: { show_icon?: boolean }): boolean {
  return config?.show_icon !== false;
}

/** 显示文字：控制徽章用 show_name；状态徽章兼容旧的 show_state。 */
export function badgeShowText(config?: { show_name?: boolean; show_state?: boolean }): boolean {
  if (!config) return true;
  if (typeof config.show_name === "boolean") return config.show_name;
  if (typeof config.show_state === "boolean") return config.show_state;
  return true;
}

/** 圆形/胶囊底色，默认显示；关闭后透明，图标颜色跟随状态。 */
export function badgeShowBackground(config?: { show_background?: boolean }): boolean {
  return config?.show_background !== false;
}

/** 徽章高度（px，横向胶囊），限制在 18–80。旧圆形大尺寸会映射到合理高度。 */
export function badgeVisualSize(
  config?: { size?: number },
  fallback: number = DEFAULT_BADGE_VISUAL_SIZE,
): number {
  const raw = config?.size;
  let value = typeof raw === "number" ? raw : Number(raw);
  // 兼容旧圆形直径配置（常见 36/56/72/96）
  if (Number.isFinite(value) && value > 48) {
    value = Math.round((value * 26) / 56);
  }
  const base = Number.isFinite(value) && value > 0 ? value : fallback;
  const fallbackSafe =
    Number.isFinite(fallback) && fallback > 0 ? fallback : DEFAULT_BADGE_VISUAL_SIZE;
  return Math.min(80, Math.max(18, Number.isFinite(base) && base > 0 ? base : fallbackSafe));
}

/** 将当前尺寸映射到最近的预设档位（用于编辑器下拉）。 */
export function nearestBadgeSizePreset(size: number): number {
  let best: number = BADGE_SIZE_PRESETS[1]?.value ?? DEFAULT_BADGE_VISUAL_SIZE;
  let bestDist = Number.POSITIVE_INFINITY;
  for (const preset of BADGE_SIZE_PRESETS) {
    const dist = Math.abs(preset.value - size);
    if (dist < bestDist) {
      best = preset.value;
      bestDist = dist;
    }
  }
  return best;
}
