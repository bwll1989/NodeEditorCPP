/** Shared visibility flags for heading + picture-elements badges. */

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
