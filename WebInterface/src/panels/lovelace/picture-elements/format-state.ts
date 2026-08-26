import type { Flow } from "../../../types";

export function formatPictureBadgeState(
  flow: Flow | undefined,
  entity: string,
  unitOverride?: string,
): { value: string; unit: string; numeric: boolean } {
  if (!entity || !flow) return { value: "—", unit: "", numeric: false };
  const state = flow.states[entity];
  if (!state) return { value: "—", unit: "", numeric: false };
  const unit = String(unitOverride ?? state.attributes.unit_of_measurement ?? "").trim();
  const raw = state.state;
  if (typeof raw === "boolean") {
    return { value: raw ? "开" : "关", unit: "", numeric: false };
  }
  if (raw === null || raw === undefined) {
    return { value: "—", unit: "", numeric: false };
  }
  if (typeof raw === "number") {
    const text = Number.isInteger(raw) ? String(raw) : raw.toFixed(1).replace(/\.0$/, "");
    return { value: text, unit, numeric: true };
  }
  const asNum = Number(raw);
  if (raw !== "" && Number.isFinite(asNum)) {
    const text = Number.isInteger(asNum) ? String(asNum) : asNum.toFixed(1).replace(/\.0$/, "");
    return { value: text, unit, numeric: true };
  }
  return { value: String(raw), unit, numeric: false };
}
