/** Theme color tokens aligned with Home Assistant */
export const THEME_COLOR_OPTIONS = [
  { value: "primary", label: "主题色" },
  { value: "accent", label: "强调色" },
  { value: "red", label: "红色" },
  { value: "pink", label: "粉色" },
  { value: "purple", label: "紫色" },
  { value: "deep-purple", label: "深紫" },
  { value: "indigo", label: "靛蓝" },
  { value: "blue", label: "蓝色" },
  { value: "light-blue", label: "浅蓝" },
  { value: "cyan", label: "青色" },
  { value: "teal", label: "蓝绿" },
  { value: "green", label: "绿色" },
  { value: "light-green", label: "浅绿" },
  { value: "lime", label: "青柠" },
  { value: "yellow", label: "黄色" },
  { value: "amber", label: "琥珀色" },
  { value: "orange", label: "橙色" },
  { value: "deep-orange", label: "深橙" },
  { value: "brown", label: "棕色" },
  { value: "grey", label: "灰色" },
  { value: "blue-grey", label: "蓝灰" },
] as const;

const THEME_SET = new Set<string>(THEME_COLOR_OPTIONS.map((c) => c.value));

export function computeCssColor(color: string): string {
  if (THEME_SET.has(color)) {
    return `var(--${color}-color, var(--primary-color))`;
  }
  return color;
}

export function themeColorLabel(value: string): string {
  return THEME_COLOR_OPTIONS.find((c) => c.value === value)?.label ?? value;
}
