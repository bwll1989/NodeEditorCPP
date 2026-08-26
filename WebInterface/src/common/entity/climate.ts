import type { FlowEntityState } from "../../types";

const HVAC_LABELS: Record<string, string> = {
  off: "关闭",
  heat: "舒适",
  cool: "制冷",
  heat_cool: "自动",
  auto: "自动",
  dry: "除湿",
  fan_only: "送风",
};

export function getHvacMode(state?: FlowEntityState): string {
  const mode = state?.attributes?.hvac_mode ?? state?.state;
  if (typeof mode === "number") return mode > 0 ? "on" : "off";
  return String(mode ?? "off").toLowerCase();
}

export function formatHvacMode(state?: FlowEntityState): string {
  const mode = getHvacMode(state);
  if (mode === "on") return "开启";
  return HVAC_LABELS[mode] ?? mode;
}

export function climateActive(state?: FlowEntityState): boolean {
  if (typeof state?.state === "number") return state.state > 0;
  return getHvacMode(state) !== "off";
}

export function getCurrentValue(state?: FlowEntityState): number | undefined {
  const current = state?.attributes?.current_temperature ?? state?.attributes?.current_value;
  if (typeof current === "number") return current;
  if (current !== undefined && current !== null) return Number(current) || undefined;
  return undefined;
}

export function getTargetTemperature(state?: FlowEntityState, fallback = 0): number {
  if (typeof state?.state === "number") return state.state;
  const attrs = state?.attributes ?? {};
  const v = attrs.temperature ?? attrs.target_temp_high ?? attrs.target_temp_low ?? attrs.value;
  if (typeof v === "number") return v;
  if (v !== undefined && v !== null) return Number(v) || fallback;
  return fallback;
}

export function getClimateMin(state?: FlowEntityState, configMin?: number): number {
  const v = state?.attributes?.min_temp ?? state?.attributes?.min;
  if (typeof v === "number") return v;
  if (configMin !== undefined && !Number.isNaN(configMin)) return configMin;
  return 0;
}

export function getClimateMax(state?: FlowEntityState, configMax?: number): number {
  const v = state?.attributes?.max_temp ?? state?.attributes?.max;
  if (typeof v === "number") return v;
  if (configMax !== undefined && !Number.isNaN(configMax)) return configMax;
  return 100;
}

export function getClimateStep(state?: FlowEntityState, configStep?: number): number {
  const v = state?.attributes?.target_temp_step ?? state?.attributes?.step;
  if (typeof v === "number") return v;
  if (configStep !== undefined && !Number.isNaN(configStep)) return configStep;
  return 1;
}

export function getClimateUnit(state?: FlowEntityState, configUnit?: string): string {
  const attrUnit = state?.attributes?.unit_of_measurement;
  if (typeof attrUnit === "string" && attrUnit.trim()) return attrUnit.trim();
  if (configUnit?.trim()) return configUnit.trim();
  return "";
}

export function formatClimateSecondary(state?: FlowEntityState, unit = ""): string {
  const mode = formatHvacMode(state);
  const current = getCurrentValue(state);
  const suffix = unit ? ` ${unit}` : "";
  const hvacMode = getHvacMode(state);

  if (current !== undefined) {
    const digits = Number.isInteger(current) ? 0 : 1;
    const modePrefix =
      hvacMode !== "off" && hvacMode !== "on" && mode !== "关闭" ? `${mode} · ` : "";
    return `${modePrefix}${current.toFixed(digits)}${suffix}`.trim();
  }

  if (typeof state?.state === "number") {
    const digits = Number.isInteger(state.state) ? 0 : 1;
    return `${state.state.toFixed(digits)}${suffix}`.trim();
  }

  return mode;
}
