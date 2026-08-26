import type { LovelacePictureElementConfig } from "../../../types";

export function parsePercent(raw: unknown, fallback = 50): number {
  if (typeof raw === "number" && Number.isFinite(raw)) return raw;
  if (typeof raw === "string") {
    const n = parseFloat(raw);
    if (Number.isFinite(n)) return n;
  }
  return fallback;
}

export function clampPercent(value: number): number {
  return Math.min(100, Math.max(0, value));
}

export function elementPosition(element: LovelacePictureElementConfig): { top: number; left: number } {
  return {
    top: parsePercent(element.style?.top, 50),
    left: parsePercent(element.style?.left, 50),
  };
}

export function withPosition(
  element: LovelacePictureElementConfig,
  top: number,
  left: number,
): LovelacePictureElementConfig {
  return {
    ...element,
    style: {
      ...element.style,
      top: `${clampPercent(top).toFixed(1)}%`,
      left: `${clampPercent(left).toFixed(1)}%`,
    },
  };
}
