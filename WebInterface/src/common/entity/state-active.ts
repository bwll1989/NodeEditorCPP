import type { FlowEntityState } from "../../types";

export function stateActive(state?: FlowEntityState): boolean {
  if (!state) return false;
  const v = state.state;
  if (typeof v === "boolean") return v;
  if (typeof v === "number") return v > 0;
  const s = String(v).toLowerCase();
  return s === "on" || s === "true" || s === "1" || s === "open" || s === "playing";
}

export function toggleValue(state?: FlowEntityState): boolean | number {
  const active = stateActive(state);
  if (typeof state?.state === "number") {
    return active ? 0 : 100;
  }
  return !active;
}
