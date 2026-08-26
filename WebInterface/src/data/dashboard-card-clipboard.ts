import type { LovelaceCardConfig } from "../types";

/** HA sessionStorage key for dashboard card clipboard */
export const DASHBOARD_CARD_CLIPBOARD_KEY = "dashboardCardClipboard";

/** Recently used card types (localStorage, survives reload) */
export const DASHBOARD_RECENT_CARD_TYPES_KEY = "dashboardRecentCardTypes";

const RECENT_MAX = 4;

const CLIPBOARD_CHANGED = "flow-dashboard-card-clipboard-changed";

export function getCardClipboard(): LovelaceCardConfig | undefined {
  try {
    const raw = sessionStorage.getItem(DASHBOARD_CARD_CLIPBOARD_KEY);
    if (!raw) return undefined;
    const parsed = JSON.parse(raw) as LovelaceCardConfig;
    if (!parsed || typeof parsed !== "object" || !parsed.type) return undefined;
    return parsed;
  } catch {
    return undefined;
  }
}

export function setCardClipboard(config: LovelaceCardConfig): void {
  try {
    sessionStorage.setItem(DASHBOARD_CARD_CLIPBOARD_KEY, JSON.stringify(config));
  } catch {
    // ignore quota / private mode
  }
  document.dispatchEvent(new CustomEvent(CLIPBOARD_CHANGED));
}

export function clearCardClipboard(): void {
  try {
    sessionStorage.removeItem(DASHBOARD_CARD_CLIPBOARD_KEY);
  } catch {
    // ignore
  }
  document.dispatchEvent(new CustomEvent(CLIPBOARD_CHANGED));
}

export function onCardClipboardChanged(listener: () => void): () => void {
  document.addEventListener(CLIPBOARD_CHANGED, listener);
  window.addEventListener("storage", listener);
  return () => {
    document.removeEventListener(CLIPBOARD_CHANGED, listener);
    window.removeEventListener("storage", listener);
  };
}

export function getRecentCardTypes(): string[] {
  try {
    const raw = localStorage.getItem(DASHBOARD_RECENT_CARD_TYPES_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw) as unknown;
    if (!Array.isArray(parsed)) return [];
    return parsed.filter((item): item is string => typeof item === "string" && Boolean(item));
  } catch {
    return [];
  }
}

export function pushRecentCardType(type: string): void {
  if (!type || type === "error" || type === "color") return;
  const next = [type, ...getRecentCardTypes().filter((t) => t !== type)].slice(0, RECENT_MAX);
  try {
    localStorage.setItem(DASHBOARD_RECENT_CARD_TYPES_KEY, JSON.stringify(next));
  } catch {
    // ignore
  }
}

/** Cards commonly suggested when opening the picker (HA suggested_cards style). */
export const DEFAULT_SUGGESTED_CARD_TYPES = [
  "heading",
  "tile",
  "sensor",
  "status",
] as const;

/** Max cards in the recommended section, including clipboard paste. */
export const RECOMMENDED_CARD_LIMIT = 4;
