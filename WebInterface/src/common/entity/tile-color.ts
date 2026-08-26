import { computeCssColor } from "../color/theme-colors";

/**
 * HA tile active color: custom color only applies when the entity is active.
 * Returns a CSS color value for --tile-color, or undefined to use theme defaults.
 */
export function computeTileColor(active: boolean, color?: unknown): string | undefined {
  if (!active) return undefined;
  const custom = color ? String(color).trim() : "";
  if (!custom) return undefined;
  return computeCssColor(custom);
}

/** Feature controls (slider/switch) color — includes HA default active colors. */
export function computeFeatureColor(
  active: boolean,
  color?: unknown,
  defaultActive?: string,
): string | undefined {
  const custom = computeTileColor(active, color);
  if (custom) return custom;
  if (!active) return undefined;
  return defaultActive ? computeCssColor(defaultActive) : undefined;
}

/** Brightness / slider cards use amber when active without a custom color (HA light state color). */
export function computeBrightnessTileColor(active: boolean, color?: unknown): string | undefined {
  return computeFeatureColor(active, color, "amber");
}

/** Switch feature color: custom when active, otherwise inherit theme via CSS. */
export function computeSwitchFeatureColor(active: boolean, color?: unknown): string | undefined {
  return computeTileColor(active, color);
}

/** Sensor icons may use color without an on/off state. */
export function computeSensorColor(color?: unknown): string | undefined {
  const custom = color ? String(color).trim() : "";
  if (!custom) return undefined;
  return computeCssColor(custom);
}
