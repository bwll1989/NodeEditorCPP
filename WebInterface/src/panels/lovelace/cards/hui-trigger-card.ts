import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, FlowValue, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { computeTileColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-card";

const PULSE_MS = 700;

@customElement("hui-trigger-card")
export class HuiTriggerCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "trigger",
      entity: "/demo/trigger",
      name: "Trigger",
      icon: "mdi:gesture-tap-button",
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  @state() private _flashing = false;

  private _lastActive = false;

  private _entityReady = false;

  private _entityUnsub?: () => void;

  private _pulseTimer?: ReturnType<typeof setTimeout>;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this.requestUpdate();
    this._syncEntitySubscription();
  }

  connectedCallback(): void {
    super.connectedCallback();
    this._syncEntitySubscription();
  }

  disconnectedCallback(): void {
    this._clearEntitySubscription();
    if (this._pulseTimer) clearTimeout(this._pulseTimer);
    super.disconnectedCallback();
  }

  protected updated(changed: import("lit").PropertyValues): void {
    if (changed.has("flow")) {
      this._syncEntitySubscription();
    }
  }

  private _clearEntitySubscription(): void {
    this._entityUnsub?.();
    this._entityUnsub = undefined;
    this._entityReady = false;
  }

  private _syncEntitySubscription(): void {
    this._clearEntitySubscription();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;

    this._entityUnsub = this.flow.subscribeEntity(entity, (state) => {
      const active = stateActive(state);
      if (this._entityReady) {
        if (active && !this._lastActive) {
          this._pulse();
        }
      } else {
        this._entityReady = true;
      }
      this._lastActive = active;
    });
  }

  private _pulse(): void {
    this._flashing = true;
    if (this._pulseTimer) clearTimeout(this._pulseTimer);
    this._pulseTimer = setTimeout(() => {
      this._flashing = false;
      this._pulseTimer = undefined;
      this.requestUpdate();
    }, PULSE_MS);
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    const rows = vertical ? 2 : 1;
    return {
      columns: 6,
      rows,
      min_columns: vertical ? 3 : 6,
      min_rows: rows,
    };
  }

  private _entityState() {
    const entity = this._config?.entity;
    if (!entity || !this.flow) return undefined;
    return this.flow.states[entity];
  }

  private async _trigger(ev?: Event): Promise<void> {
    ev?.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    await this.flow.callService(entity, true as FlowValue);
    this._pulse();
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = this._entityState();
    const entityActive = stateActive(state);
    const active = this._flashing || entityActive;
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "Trigger",
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:gesture-tap-button");
    const vertical = Boolean(this._config.vertical);
    const stateText = active ? "已触发" : "未触发";
    const colorStyle = styleMap({
      "--tile-color": computeTileColor(active, this._config.color),
    });

    return html`
      <ha-card
        class=${classMap({ active, pulse: this._flashing })}
        style=${colorStyle}
        tabindex="0"
        @click=${this._trigger}
        @keydown=${(ev: KeyboardEvent) => {
          if (ev.key === "Enter" || ev.key === " ") {
            ev.preventDefault();
            void this._trigger();
          }
        }}
      >
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon
            slot="icon"
            .icon=${icon}
            .active=${active}
            interactive
          ></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${stateText}
          ></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `;
  }

  static styles = [
    tileCardStyle,
    tileCardHostStyle,
    css`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        cursor: pointer;
        transition:
          box-shadow 200ms ease-in-out,
          border-color 200ms ease-in-out;
      }
      ha-card.pulse {
        transition:
          box-shadow 120ms ease-in-out,
          border-color 120ms ease-in-out;
      }
      ha-card:hover {
        box-shadow:
          var(--ha-card-box-shadow),
          0 2px 6px rgba(0, 0, 0, 0.06);
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-trigger-card": HuiTriggerCard;
  }
}
