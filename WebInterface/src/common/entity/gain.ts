import type { FlowEntityState } from "../../types";

export const DEFAULT_GAIN_MIN = -60;
export const DEFAULT_GAIN_MAX = 12;
export const DEFAULT_GAIN_STEP = 1;

export function getGainValue(state?: FlowEntityState, fallback = 0): number {
  if (typeof state?.state === "number") return state.state;
  const v = state?.attributes?.value ?? state?.attributes?.gain;
  if (typeof v === "number") return v;
  if (v !== undefined && v !== null) return Number(v) || fallback;
  return fallback;
}

export function getGainMin(state?: FlowEntityState, configMin?: number): number {
  const v = state?.attributes?.min ?? state?.attributes?.min_value;
  if (typeof v === "number") return v;
  if (configMin !== undefined && !Number.isNaN(configMin)) return configMin;
  return DEFAULT_GAIN_MIN;
}

export function getGainMax(state?: FlowEntityState, configMax?: number): number {
  const v = state?.attributes?.max ?? state?.attributes?.max_value;
  if (typeof v === "number") return v;
  if (configMax !== undefined && !Number.isNaN(configMax)) return configMax;
  return DEFAULT_GAIN_MAX;
}

export function getGainStep(state?: FlowEntityState, configStep?: number): number {
  const v = state?.attributes?.step;
  if (typeof v === "number") return v;
  if (configStep !== undefined && !Number.isNaN(configStep)) return configStep;
  return DEFAULT_GAIN_STEP;
}

export function isMuted(
  gainState?: FlowEntityState,
  muteState?: FlowEntityState,
): boolean {
  if (muteState) {
    if (typeof muteState.state === "boolean") return muteState.state;
    return String(muteState.state).toLowerCase() === "true" || muteState.state === 1;
  }
  if (typeof gainState?.attributes?.is_muted === "boolean") {
    return gainState.attributes.is_muted;
  }
  return false;
}

export function formatGainSecondary(muted: boolean): string {
  return muted ? "静音" : "开启";
}

export function digitsForStep(step: number): number {
  if (step >= 1) return 0;
  const text = String(step);
  const dot = text.indexOf(".");
  return dot >= 0 ? text.length - dot - 1 : 0;
}
