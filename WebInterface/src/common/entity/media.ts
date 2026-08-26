import type { FlowEntityState, FlowValue } from "../../types";

export type MediaPlayback = "play" | "stop";

const PLAYING_STATES = new Set([
  "playing",
  "play",
  "on",
  "true",
  "running",
  "started",
]);

export function isMediaPlaying(state?: FlowEntityState): boolean {
  if (!state) return false;
  const value = state.state;
  if (typeof value === "boolean") return value;
  if (typeof value === "number") return value > 0;
  return PLAYING_STATES.has(String(value).toLowerCase());
}

export function getMediaPlayback(state?: FlowEntityState): MediaPlayback {
  return isMediaPlaying(state) ? "play" : "stop";
}

export function formatMediaSecondary(state?: FlowEntityState): string {
  return isMediaPlaying(state) ? "正在播放" : "已停止";
}

export function resolveMediaCommand(
  action: MediaPlayback,
  config?: Record<string, unknown>,
): FlowValue {
  const key = action === "play" ? "play_value" : "stop_value";
  if (key in (config ?? {})) {
    return parseCommandValue(config?.[key]);
  }
  return action === "play";
}

function parseCommandValue(raw: unknown): FlowValue {
  if (typeof raw === "boolean" || typeof raw === "number") return raw;
  if (raw === undefined || raw === null || raw === "") return true;
  const text = String(raw).trim();
  if (text === "true") return true;
  if (text === "false") return false;
  const num = Number(text);
  if (!Number.isNaN(num) && text !== "") return num;
  return text;
}
