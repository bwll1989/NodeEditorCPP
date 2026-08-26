export type XyPoint = { x: number; y: number };

export const XY_PAD_DEFAULTS = {
  x_min: 0,
  x_max: 1,
  y_min: 0,
  y_max: 1,
  step: 0.01,
  invert_y: true,
  snap_center: false,
  show_grid: true,
} as const;

function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

export function stepValue(value: number, step: number, min: number, max: number): number {
  if (!Number.isFinite(step) || step <= 0) return clamp(value, min, max);
  const stepped = Math.round(value / step) * step;
  // Avoid float noise like 0.30000000004
  const decimals = String(step).includes(".") ? (String(step).split(".")[1]?.length ?? 0) : 0;
  const rounded = Number(stepped.toFixed(Math.min(decimals + 2, 8)));
  return clamp(rounded, min, max);
}

export function parseXyValue(value: unknown): XyPoint | undefined {
  if (Array.isArray(value)) {
    if (value.length < 2) return undefined;
    const x = Number(value[0]);
    const y = Number(value[1]);
    if (Number.isNaN(x) || Number.isNaN(y)) return undefined;
    return { x, y };
  }

  if (value && typeof value === "object") {
    const record = value as Record<string, unknown>;
    const x = Number(record.x ?? record.X ?? record.left);
    const y = Number(record.y ?? record.Y ?? record.top);
    if (Number.isNaN(x) || Number.isNaN(y)) return undefined;
    return { x, y };
  }

  if (typeof value === "string" && value.trim()) {
    try {
      const parsed = JSON.parse(value) as unknown;
      return parseXyValue(parsed);
    } catch {
      const parts = value.split(/[,;\s]+/).map((part) => Number(part.trim()));
      if (parts.length >= 2 && !parts.some((n) => Number.isNaN(n))) {
        return { x: parts[0], y: parts[1] };
      }
    }
  }

  return undefined;
}

export function xyToFlowValue(point: XyPoint): number[] {
  return [Number(point.x.toFixed(6)), Number(point.y.toFixed(6))];
}

export function formatXyDisplay(point: XyPoint, digits = 2): string {
  return `${point.x.toFixed(digits)}, ${point.y.toFixed(digits)}`;
}

export function normalizeXyPoint(
  point: XyPoint,
  options: {
    xMin: number;
    xMax: number;
    yMin: number;
    yMax: number;
    step: number;
  },
): XyPoint {
  return {
    x: stepValue(point.x, options.step, options.xMin, options.xMax),
    y: stepValue(point.y, options.step, options.yMin, options.yMax),
  };
}

export function centerXyPoint(xMin: number, xMax: number, yMin: number, yMax: number): XyPoint {
  return {
    x: (xMin + xMax) / 2,
    y: (yMin + yMax) / 2,
  };
}
