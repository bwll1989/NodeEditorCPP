import type { FlowValue } from "../../../types";

/** Parse editor text into the value written by a button heading badge. */
export function parseBadgeWriteValue(raw: unknown): FlowValue {
  if (typeof raw === "boolean" || typeof raw === "number") return raw;
  if (raw === null || raw === undefined) return true;
  const text = String(raw).trim();
  if (text === "") return true;
  if (text === "true") return true;
  if (text === "false") return false;
  const num = Number(text);
  if (text !== "" && Number.isFinite(num) && String(num) === text) return num;
  return text;
}
