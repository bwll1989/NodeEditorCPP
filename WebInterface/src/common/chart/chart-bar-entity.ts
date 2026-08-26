import type { Flow, FlowEntityState } from "../../types";
import { numOr } from "./point2d";

export interface BarEntityConfig {
  entity: string;
  name?: string;
  color?: string;
}

export interface BarItem {
  name: string;
  value: number;
  color?: string;
}

export function normalizeBarEntities(raw: unknown): BarEntityConfig[] {
  if (!Array.isArray(raw)) return [];
  const result: BarEntityConfig[] = [];
  for (const item of raw) {
    if (!item || typeof item !== "object") continue;
    const record = item as Record<string, unknown>;
    const entity = String(record.entity ?? "").trim();
    if (!entity) continue;
    result.push({
      entity,
      ...(record.name ? { name: String(record.name) } : {}),
      ...(record.color ? { color: String(record.color) } : {}),
    });
  }
  return result;
}

function stateToBarValue(state: FlowEntityState | undefined): number {
  if (!state) return 0;
  const raw = state.state;
  if (typeof raw === "boolean") return raw ? 1 : 0;
  return numOr(raw);
}

function barLabel(config: BarEntityConfig, state: FlowEntityState | undefined): string {
  const custom = config.name?.trim();
  if (custom) return custom;
  const friendly = state?.attributes?.friendly_name;
  if (typeof friendly === "string" && friendly.trim()) return friendly.trim();
  const tail = config.entity.split("/").filter(Boolean).pop();
  return tail ?? config.entity;
}

/** Subscribe to multiple entities; each bar shows that address's live value. */
export class ChartMultiBarTracker {
  bars: BarItem[] = [];

  private _configs: BarEntityConfig[] = [];

  private _flow?: Flow;

  private _defaultColor = "#03a9f4";

  private _unsubs: Array<() => void> = [];

  private _onChange?: () => void;

  bind(
    flow: Flow | undefined,
    configs: BarEntityConfig[],
    defaultColor: string,
    onChange: () => void,
  ): void {
    this.dispose();
    this._flow = flow;
    this._configs = configs;
    this._defaultColor = defaultColor || "#03a9f4";
    this._onChange = onChange;

    if (!flow) {
      this._rebuild();
      return;
    }

    configs.forEach((config) => {
      const entity = config.entity.trim();
      if (!entity) return;
      this._unsubs.push(
        flow.subscribeEntity(entity, () => {
          this._rebuild();
        }),
      );
    });

    this._rebuild();
  }

  dispose(): void {
    this._unsubs.forEach((unsub) => unsub());
    this._unsubs = [];
    this._configs = [];
    this._flow = undefined;
    this._onChange = undefined;
    this.bars = [];
  }

  private _rebuild(): void {
    const flow = this._flow;
    this.bars = this._configs.map((config) => {
      const entity = config.entity.trim();
      const state = flow?.states[entity];
      return {
        name: barLabel(config, state),
        value: stateToBarValue(state),
        color: config.color?.trim() || this._defaultColor,
      };
    });
    this._onChange?.();
  }
}
