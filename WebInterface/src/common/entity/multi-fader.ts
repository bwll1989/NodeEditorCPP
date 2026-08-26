/** Helpers for multi-fader / N-dim vector cards. */

export const MULTI_FADER_DEFAULTS = {
  count: 4,
  min: 0,
  max: 1,
  step: 0.01,
  max_count: 16,
} as const;

function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

export function clampFaderCount(count: unknown, fallback = MULTI_FADER_DEFAULTS.count): number {
  const n = Number(count);
  if (!Number.isFinite(n)) return fallback;
  return Math.max(1, Math.min(MULTI_FADER_DEFAULTS.max_count, Math.round(n)));
}

export function stepFaderValue(value: number, step: number, min: number, max: number): number {
  if (!Number.isFinite(step) || step <= 0) return clamp(value, min, max);
  const stepped = Math.round(value / step) * step;
  const decimals = String(step).includes(".") ? (String(step).split(".")[1]?.length ?? 0) : 0;
  return clamp(Number(stepped.toFixed(Math.min(decimals + 2, 8))), min, max);
}

export function parseFaderLabels(value: unknown, count: number): string[] {
  let raw: string[] = [];
  if (Array.isArray(value)) {
    raw = value.map((item) => String(item ?? "").trim());
  } else if (typeof value === "string" && value.trim()) {
    raw = value.split(/[,???|]/).map((part) => part.trim());
  }
  return Array.from({ length: count }, (_, i) => raw[i] || String(i));
}

export function parseVectorValue(
  value: unknown,
  count: number,
  options: { min: number; max: number; step: number; fill?: number },
): number[] {
  const fill =
    options.fill ??
    stepFaderValue((options.min + options.max) / 2, options.step, options.min, options.max);
  const base = Array.from({ length: count }, () => fill);

  let nums: number[] | undefined;
  if (Array.isArray(value)) {
    nums = value.map((item) => Number(item));
  } else if (typeof value === "string" && value.trim()) {
    try {
      const parsed = JSON.parse(value) as unknown;
      if (Array.isArray(parsed)) nums = parsed.map((item) => Number(item));
    } catch {
      const parts = value.split(/[,;\s]+/).map((part) => Number(part.trim()));
      if (parts.length && !parts.some((n) => Number.isNaN(n))) nums = parts;
    }
  }

  if (!nums) return base;

  for (let i = 0; i < count; i += 1) {
    const n = nums[i];
    base[i] = Number.isFinite(n)
      ? stepFaderValue(n, options.step, options.min, options.max)
      : fill;
  }
  return base;
}

export function vectorToFlowValue(values: number[]): number[] {
  return values.map((value) => Number(value.toFixed(6)));
}

export function formatVectorDisplay(values: number[], digits = 2): string {
  return values.map((value) => value.toFixed(digits)).join(", ");
}
