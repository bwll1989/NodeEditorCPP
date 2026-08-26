import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type {
  Flow,
  FlowEntityState,
  LovelaceCard,
  LovelaceCardConfig,
  LovelaceGridOptions,
  LovelaceStatusEntityConfig,
} from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { computeSensorColor } from "../../../common/entity/tile-color";
import "../../../components/ha-card";
import "../../../components/ha-icon";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";

export type StatusKind = "ok" | "error" | "unknown";

const OK_VALUES = new Set([
  "on",
  "true",
  "1",
  "ok",
  "normal",
  "connected",
  "open",
  "playing",
  "ready",
  "正常",
]);

const ERROR_VALUES = new Set([
  "off",
  "false",
  "0",
  "error",
  "fail",
  "failed",
  "disconnected",
  "closed",
  "unavailable",
  "异常",
  "错误",
  "断开",
]);

export function normalizeStatusEntities(
  entities: unknown,
): LovelaceStatusEntityConfig[] {
  if (!Array.isArray(entities)) return [];
  return entities
    .map((item) => {
      if (!item || typeof item !== "object") return null;
      const row = item as Record<string, unknown>;
      const entity = String(row.entity ?? "").trim();
      if (!entity) return null;
      const name = row.name != null ? String(row.name) : undefined;
      const icon = row.icon != null ? String(row.icon) : undefined;
      return {
        entity,
        ...(name ? { name } : {}),
        ...(icon ? { icon } : {}),
      } satisfies LovelaceStatusEntityConfig;
    })
    .filter((item): item is LovelaceStatusEntityConfig => item !== null);
}

export function resolveStatusKind(state?: FlowEntityState): StatusKind {
  if (!state || state.state === null || state.state === undefined || state.state === "") {
    return "unknown";
  }
  const v = state.state;
  if (typeof v === "boolean") return v ? "ok" : "error";
  if (typeof v === "number") return v > 0 ? "ok" : "error";
  if (Array.isArray(v)) return v.length > 0 ? "ok" : "error";
  const s = String(v).toLowerCase().trim();
  if (OK_VALUES.has(s)) return "ok";
  if (ERROR_VALUES.has(s)) return "error";
  return stateActive(state) ? "ok" : "error";
}

function statusLabel(kind: StatusKind): string {
  if (kind === "ok") return "正常";
  if (kind === "error") return "异常";
  return "—";
}

@customElement("hui-status-card")
export class HuiStatusCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "status",
      name: "Flow 连接",
      icon: "mdi:lan-connect",
      color: "green",
      entities: [
        { entity: "/demo/status_ws", name: "connected", icon: "mdi:lan-connect" },
        { entity: "/demo/status_tcp", name: "tcp连接", icon: "mdi:lan-connect" },
        { entity: "/demo/status_udp", name: "connected", icon: "mdi:lan-connect" },
        { entity: "/demo/status_osc", name: "connected", icon: "mdi:lan-connect" },
      ],
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  private _unsubs: Array<() => void> = [];

  private _subscribedKey = "";

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this._syncSubscriptions();
    this.requestUpdate();
  }

  connectedCallback(): void {
    super.connectedCallback();
    this._syncSubscriptions();
  }

  disconnectedCallback(): void {
    this._clearSubscriptions();
    super.disconnectedCallback();
  }

  protected updated(changed: PropertyValues): void {
    if (changed.has("flow") || changed.has("_config")) {
      this._syncSubscriptions();
    }
  }

  private _clearSubscriptions(): void {
    this._unsubs.forEach((unsub) => unsub());
    this._unsubs = [];
    this._subscribedKey = "";
  }

  private _syncSubscriptions(): void {
    const flow = this.flow;
    const entities = normalizeStatusEntities(this._config?.entities);
    const key = `${flow ? "1" : "0"}:${entities.map((e) => e.entity).join("\0")}`;
    if (key === this._subscribedKey && this._unsubs.length > 0) return;
    this._clearSubscriptions();
    if (!flow) return;
    this._subscribedKey = key;
    entities.forEach((item) => {
      this._unsubs.push(
        flow.subscribeEntity(item.entity, () => {
          this.requestUpdate();
        }),
      );
    });
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    return {
      columns: vertical ? 3 : 6,
      rows: "auto",
      min_columns: vertical ? 3 : 6,
      min_rows: 2,
    };
  }

  protected render() {
    if (!this._config) return nothing;
    void this.flow?.statesRevision;

    const entities = normalizeStatusEntities(this._config.entities);
    const name = String(this._config.name ?? "状态");
    const icon = String(this._config.icon ?? "mdi:lan-connect");
    const vertical = Boolean(this._config.vertical);
    const listColumns = Number(this._config.list_columns) === 2 ? 2 : 1;
    const okColor =
      computeSensorColor(this._config.color) ?? "var(--green-color, #4caf50)";

    const kinds = entities.map((item) =>
      resolveStatusKind(this.flow?.states[item.entity]),
    );
    const okCount = kinds.filter((k) => k === "ok").length;
    const total = entities.length;
    const allOk = total > 0 && okCount === total;
    const anyError = kinds.some((k) => k === "error");
    const summaryKind: StatusKind =
      total === 0 ? "unknown" : allOk ? "ok" : anyError ? "error" : "unknown";
    const summaryText =
      total === 0
        ? "无绑定"
        : `${okCount}/${total} ${summaryKind === "ok" ? "正常" : summaryKind === "error" ? "异常" : "未知"}`;

    const accentColor =
      summaryKind === "error"
        ? "var(--red-color, #f44336)"
        : summaryKind === "unknown"
          ? "var(--state-inactive-color, #9e9e9e)"
          : okColor;

    const colorStyle = styleMap({
      "--status-ok": okColor,
      "--status-accent": accentColor,
      "--tile-color": accentColor,
    });

    return html`
      <ha-card
        class=${classMap({
          vertical,
          ok: summaryKind === "ok",
          error: summaryKind === "error",
          unknown: summaryKind === "unknown",
        })}
        style=${colorStyle}
      >
        <ha-tile-container ?vertical=${vertical} expand-features>
          <ha-tile-icon slot="icon" .icon=${icon}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${summaryText}
          ></ha-tile-info>
          <div slot="features" class=${classMap({ list: true, "cols-2": listColumns === 2 })}>
            ${entities.length === 0
              ? html`<p class="empty">添加地址后显示各项状态</p>`
              : entities.map((item, index) => {
                  const state = this.flow?.states[item.entity];
                  const kind = kinds[index] ?? "unknown";
                  const label =
                    item.name?.trim() ||
                    String(state?.attributes?.friendly_name ?? "") ||
                    item.entity;
                  const rowIcon =
                    item.icon?.trim() ||
                    String(state?.attributes?.icon ?? "") ||
                    icon;
                  return html`
                    <div
                      class=${classMap({
                        row: true,
                        ok: kind === "ok",
                        error: kind === "error",
                        unknown: kind === "unknown",
                      })}
                    >
                      <ha-icon class="row-icon" .icon=${rowIcon}></ha-icon>
                      <span class="row-name">${label}</span>
                      <span class="row-status">
                        <span class="dot"></span>
                        <span class="status-text">${statusLabel(kind)}</span>
                      </span>
                    </div>
                  `;
                })}
          </div>
        </ha-tile-container>
      </ha-card>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
      --status-ok: var(--green-color, #4caf50);
      --status-error: var(--red-color, #f44336);
      --status-unknown: var(--state-inactive-color, #9e9e9e);
      --status-accent: var(--status-ok);
    }

    ha-card {
      height: 100%;
      --tile-color: var(--status-accent);
    }

    ha-tile-icon {
      --tile-icon-color: var(--status-accent);
    }

    ha-card.ok ha-tile-info {
      --ha-tile-info-secondary-color: var(--status-ok);
    }

    ha-card.error ha-tile-info {
      --ha-tile-info-secondary-color: var(--status-error);
    }

    ha-card.unknown ha-tile-info {
      --ha-tile-info-secondary-color: var(--status-unknown);
    }

    ha-tile-info {
      --ha-tile-info-secondary-color: var(--secondary-text-color);
    }

    .list {
      display: grid;
      grid-template-columns: 1fr;
      gap: 8px;
      width: 100%;
      box-sizing: border-box;
      min-height: 0;
    }

    .list.cols-2 {
      grid-template-columns: 1fr 1fr;
    }

    .empty {
      margin: 0;
      font-size: 12px;
      color: var(--secondary-text-color);
    }

    .row {
      display: flex;
      align-items: center;
      gap: 10px;
      min-height: 40px;
      padding: 8px 12px;
      border-radius: 10px;
      background: var(--ha-color-form-background, #f5f5f5);
      box-sizing: border-box;
      --row-status-color: var(--status-unknown);
    }

    .row.ok {
      --row-status-color: var(--status-ok);
    }

    .row.error {
      --row-status-color: var(--status-error);
    }

    .row-icon {
      flex-shrink: 0;
      --mdc-icon-size: 18px;
      width: 18px;
      height: 18px;
      color: var(--row-status-color);
    }

    .row-name {
      flex: 1;
      min-width: 0;
      font-size: var(--ha-font-size-s, 12px);
      color: var(--primary-text-color);
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .row-status {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      flex-shrink: 0;
    }

    .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--row-status-color);
    }

    .status-text {
      font-size: var(--ha-font-size-s, 12px);
      color: var(--secondary-text-color);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-status-card": HuiStatusCard;
  }
}
