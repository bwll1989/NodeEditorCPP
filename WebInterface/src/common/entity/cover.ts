import type { FlowEntityState } from "../../types";
import { stateActive } from "./state-active";

export const DEFAULT_COVER_POSITION_COUNT = 4;

export function buildCoverPositions(count = DEFAULT_COVER_POSITION_COUNT): number[] {
  const safeCount = Math.max(1, Math.min(12, Math.round(count) || DEFAULT_COVER_POSITION_COUNT));
  // 档位从 0 开始：0, 1, 2, …（count=4 → 0/1/2/3）
  return Array.from({ length: safeCount }, (_, index) => index);
}

export const DEFAULT_COVER_POSITIONS = buildCoverPositions(DEFAULT_COVER_POSITION_COUNT);

export function resolveCoverPositions(config?: Record<string, unknown>): number[] {
  const configured = config?.positions;
  if (Array.isArray(configured) && configured.length > 0) {
    const values = configured.map((value) => Number(value)).filter((value) => !Number.isNaN(value));
    if (values.length > 0) return values;
  }
  return buildCoverPositions(Number(config?.position_count ?? DEFAULT_COVER_POSITION_COUNT));
}

export function getCoverPosition(state?: FlowEntityState, positions = DEFAULT_COVER_POSITIONS): number {
  const pos = state?.attributes?.current_position ?? state?.attributes?.position;
  if (typeof pos === "number") return Math.round(pos);
  if (pos !== undefined && pos !== null) return Math.round(Number(pos) || 0);
  if (typeof state?.state === "number") return Math.round(state.state);
  if (stateActive(state)) return positions[positions.length - 1] ?? 0;
  return positions[0] ?? 0;
}

export function coverActive(state?: FlowEntityState, positions = DEFAULT_COVER_POSITIONS): boolean {
  const position = getCoverPosition(state, positions);
  const closedValue = positions[0] ?? 0;
  return position > closedValue;
}

export function formatCoverSecondary(
  state?: FlowEntityState,
  positions = DEFAULT_COVER_POSITIONS,
): string {
  const pos = getCoverPosition(state, positions);
  const closedValue = positions[0] ?? 0;
  const label = pos > closedValue ? "已打开" : "已关闭";
  return `${label} · ${pos}`;
}
