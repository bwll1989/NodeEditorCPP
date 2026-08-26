export type RgbaColor = [number, number, number, number];

function clamp01(value: number): number {
  return Math.min(1, Math.max(0, value));
}

function normalizeChannel(value: number, asByte: boolean): number {
  const n = Number(value);
  if (Number.isNaN(n)) return 0;
  return asByte ? clamp01(n / 255) : clamp01(n);
}

export function parseHexColor(input: string): RgbaColor | undefined {
  const raw = input.trim().replace(/^#/, "");
  if (!/^[0-9a-f]{3,8}$/i.test(raw)) return undefined;

  const expand = (ch: string) => ch + ch;
  let hex = raw;
  if (hex.length === 3 || hex.length === 4) {
    hex = [...hex].map(expand).join("");
  }
  if (hex.length !== 6 && hex.length !== 8) return undefined;

  const r = Number.parseInt(hex.slice(0, 2), 16) / 255;
  const g = Number.parseInt(hex.slice(2, 4), 16) / 255;
  const b = Number.parseInt(hex.slice(4, 6), 16) / 255;
  const a = hex.length === 8 ? Number.parseInt(hex.slice(6, 8), 16) / 255 : 1;
  return [r, g, b, a];
}

export function parseRgbaValue(value: unknown): RgbaColor | undefined {
  if (Array.isArray(value)) {
    const nums = value.map((item) => Number(item));
    if (nums.length < 3 || nums.some((item) => Number.isNaN(item))) return undefined;
    const asByte = nums.some((item) => item > 1);
    return [
      normalizeChannel(nums[0], asByte),
      normalizeChannel(nums[1], asByte),
      normalizeChannel(nums[2], asByte),
      normalizeChannel(nums.length >= 4 ? nums[3] : 1, asByte),
    ];
  }

  if (typeof value === "string" && value.trim()) {
    return parseHexColor(value.trim());
  }

  if (value && typeof value === "object") {
    const record = value as Record<string, unknown>;
    const r = record.r ?? record.red ?? record.x;
    const g = record.g ?? record.green ?? record.y;
    const b = record.b ?? record.blue ?? record.z;
    if (r !== undefined && g !== undefined && b !== undefined) {
      const asByte = [r, g, b, record.a ?? record.alpha ?? record.w]
        .map((item) => Number(item))
        .some((item) => !Number.isNaN(item) && item > 1);
      return [
        normalizeChannel(Number(r), asByte),
        normalizeChannel(Number(g), asByte),
        normalizeChannel(Number(b), asByte),
        normalizeChannel(Number(record.a ?? record.alpha ?? record.w ?? 1), asByte),
      ];
    }
  }

  return undefined;
}

export function rgbaToCss(rgba: RgbaColor): string {
  const [r, g, b, a] = rgba;
  const rr = Math.round(r * 255);
  const gg = Math.round(g * 255);
  const bb = Math.round(b * 255);
  if (a >= 0.999) return `rgb(${rr}, ${gg}, ${bb})`;
  return `rgba(${rr}, ${gg}, ${bb}, ${a.toFixed(3)})`;
}

export function rgbaToHex(rgba: RgbaColor, includeAlpha = false): string {
  const toByte = (value: number) =>
    Math.round(clamp01(value) * 255)
      .toString(16)
      .padStart(2, "0");
  const base = `#${toByte(rgba[0])}${toByte(rgba[1])}${toByte(rgba[2])}`;
  return includeAlpha ? `${base}${toByte(rgba[3])}` : base;
}

export function rgbaToInputHex(rgba: RgbaColor): string {
  return rgbaToHex(rgba, false).toUpperCase();
}

export function inputHexToRgba(hex: string, alpha = 1): RgbaColor {
  const parsed = parseHexColor(hex);
  if (!parsed) return [0, 0, 0, alpha];
  return [parsed[0], parsed[1], parsed[2], alpha];
}

export function rgbaToFlowValue(rgba: RgbaColor): number[] {
  return rgba.map((value) => Number(value.toFixed(4)));
}

export function formatRgbaDisplay(rgba: RgbaColor): string {
  return rgbaToHex(rgba, rgba[3] < 0.999).toUpperCase();
}

/** 根据亮度返回图标前景色，保证在实色背景上可读 */
export function contrastingIconColor(rgba: RgbaColor): string {
  const luminance = 0.299 * rgba[0] + 0.587 * rgba[1] + 0.114 * rgba[2];
  return luminance > 0.58 ? "#1e293b" : "#ffffff";
}

export type HsvaColor = { h: number; s: number; v: number; a: number };

export function rgbaToHsva(rgba: RgbaColor): HsvaColor {
  const [r, g, b, a] = rgba.map(clamp01) as RgbaColor;
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const d = max - min;
  let h = 0;
  if (d > 1e-6) {
    if (max === r) h = ((g - b) / d) % 6;
    else if (max === g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h *= 60;
    if (h < 0) h += 360;
  }
  const s = max <= 1e-6 ? 0 : d / max;
  return { h, s, v: max, a };
}

export function hsvaToRgba(hsva: HsvaColor): RgbaColor {
  const h = ((hsva.h % 360) + 360) % 360;
  const s = clamp01(hsva.s);
  const v = clamp01(hsva.v);
  const a = clamp01(hsva.a);
  const c = v * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = v - c;
  let r = 0;
  let g = 0;
  let b = 0;
  if (h < 60) [r, g, b] = [c, x, 0];
  else if (h < 120) [r, g, b] = [x, c, 0];
  else if (h < 180) [r, g, b] = [0, c, x];
  else if (h < 240) [r, g, b] = [0, x, c];
  else if (h < 300) [r, g, b] = [x, 0, c];
  else [r, g, b] = [c, 0, x];
  return [clamp01(r + m), clamp01(g + m), clamp01(b + m), a];
}

/** Flow HSV node uses [h,s,v] with all channels in 0–1 (h is fraction of 360°). */
export function parseHsvaValue(value: unknown): HsvaColor | undefined {
  if (Array.isArray(value)) {
    if (value.length < 3) return undefined;
    const hRaw = Number(value[0]);
    const s = Number(value[1]);
    const v = Number(value[2]);
    if ([hRaw, s, v].some((n) => Number.isNaN(n))) return undefined;
    // Accept either 0–1 hue (Flow) or 0–360 degrees.
    const h = hRaw > 1 ? ((hRaw % 360) + 360) % 360 : clamp01(hRaw) * 360;
    return {
      h,
      s: clamp01(s),
      v: clamp01(v),
      a: value.length >= 4 ? clamp01(Number(value[3])) : 1,
    };
  }

  if (value && typeof value === "object") {
    const record = value as Record<string, unknown>;
    const hRaw = Number(record.h ?? record.hue ?? record.H);
    const s = Number(record.s ?? record.saturation ?? record.S);
    const v = Number(record.v ?? record.value ?? record.V);
    if ([hRaw, s, v].some((n) => Number.isNaN(n))) return undefined;
    const h = hRaw > 1 ? ((hRaw % 360) + 360) % 360 : clamp01(hRaw) * 360;
    return {
      h,
      s: clamp01(s),
      v: clamp01(v),
      a: clamp01(Number(record.a ?? record.alpha ?? 1)),
    };
  }

  return undefined;
}

/** Write HSV as Flow 0–1 vector [h,s,v]. */
export function hsvaToFlowValue(hsva: HsvaColor): number[] {
  return [
    Number((clamp01(hsva.h / 360)).toFixed(6)),
    Number(clamp01(hsva.s).toFixed(6)),
    Number(clamp01(hsva.v).toFixed(6)),
  ];
}

export function formatHsvaDisplay(hsva: HsvaColor): string {
  const h = Math.round(((hsva.h % 360) + 360) % 360);
  const s = Math.round(clamp01(hsva.s) * 100);
  const v = Math.round(clamp01(hsva.v) * 100);
  return `${h}° ${s}% ${v}%`;
}

export const DEFAULT_RGBA: RgbaColor = [0.12, 0.53, 0.9, 1];
