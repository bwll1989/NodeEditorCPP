import type { FlowValue } from "../../../types";
import {
  normalizeTapAction,
  parseBadgeWriteValue,
  type NormalizedTapActionKind,
  type RawBadgeConfig,
} from "./badge-model";

export type { RawBadgeConfig };
export { parseBadgeWriteValue };

export function formatBadgeWriteValue(raw: FlowValue | undefined, fallback: FlowValue): string {
  const value = raw === undefined ? fallback : raw;
  if (typeof value === "boolean") return value ? "true" : "false";
  if (value === null) return "null";
  return String(value);
}

/** @deprecated 使用 normalizeTapAction / badge-model */
export function badgeControlAction(
  config?: RawBadgeConfig,
): Extract<NormalizedTapActionKind, "trigger" | "switch" | "rw"> {
  const action = normalizeTapAction(config ?? {}).action;
  if (action === "switch" || action === "rw") return action;
  return "trigger";
}

export function badgePressValue(config?: RawBadgeConfig): FlowValue {
  const tap = normalizeTapAction(config ?? {});
  if (tap.action === "switch") return true;
  return tap.press_value === undefined ? true : parseBadgeWriteValue(tap.press_value);
}

export function badgeReleaseValue(config?: RawBadgeConfig): FlowValue {
  const tap = normalizeTapAction(config ?? {});
  if (tap.action === "switch") return false;
  return tap.release_value === undefined ? false : parseBadgeWriteValue(tap.release_value);
}
