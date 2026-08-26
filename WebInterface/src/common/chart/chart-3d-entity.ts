import type { Flow, FlowEntityState } from "../../types";
import { applyIncoming, coercePoints, type Point3 } from "./point3d";

export class Chart3dPointTracker {
  points: Point3[] = [];

  private _lastChanged = 0;

  private _entity = "";

  private _unsub?: () => void;

  private _maxPoints = 5000;

  private _onChange?: () => void;

  bind(
    flow: Flow | undefined,
    entity: string | undefined,
    maxPoints: number,
    onChange: () => void,
  ): void {
    this.dispose();
    this._onChange = onChange;
    this._maxPoints = Number.isFinite(maxPoints) && maxPoints > 0 ? Math.floor(maxPoints) : 5000;
    this._entity = entity ?? "";
    if (!flow || !entity) return;

    let first = true;
    this._unsub = flow.subscribeEntity(entity, (state) => {
      if (!state) return;
      this._consumeState(state, first);
      first = false;
    });
  }

  dispose(): void {
    this._unsub?.();
    this._unsub = undefined;
    this._lastChanged = 0;
    this._entity = "";
    this.points = [];
  }

  resetForEntity(entity: string | undefined): void {
    if (entity !== this._entity) {
      this.points = [];
      this._lastChanged = 0;
    }
  }

  private _consumeState(state: FlowEntityState, isFirst: boolean): void {
    if (state.last_changed <= this._lastChanged) return;

    if (isFirst && state.attributes?.points) {
      this.points = coercePoints(state.attributes.points);
    }

    const raw = state.state;
    if (raw !== null && raw !== undefined && raw !== "") {
      this.points = applyIncoming(this.points, raw, this._maxPoints);
    }

    this._lastChanged = state.last_changed;
    this._onChange?.();
  }
}

export const CHART3D_DEFAULTS = {
  bg_color: "",
  max_points: 5000,
  line_width: 3,
  line_color: "#03a9f4",
  symbol_size: 8,
  point_color: "#03a9f4",
  x_label: "X",
  y_label: "Y",
  z_label: "Z",
  show_axes: true,
  auto_rotate: false,
} as const;
