import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import type { MediaPlayback } from "../../../common/entity/media";
import {
  formatMediaSecondary,
  getMediaPlayback,
  isMediaPlaying,
  resolveMediaCommand,
} from "../../../common/entity/media";
import { computeFeatureColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-control-media-playback";
import "../../../components/ha-card";

@customElement("hui-media-card")
export class HuiMediaCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "media",
      entity: "/demo/media",
      name: "媒体播放器",
      icon: "mdi:cast-audio",
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    const rows = vertical ? 3 : 2;
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

  private async _onPlaybackChange(ev: CustomEvent<{ action: MediaPlayback }>): Promise<void> {
    ev.stopPropagation();
    const entity = this._config?.entity;
    if (!entity || !this.flow) return;
    const command = resolveMediaCommand(
      ev.detail.action,
      this._config as Record<string, unknown> | undefined,
    );
    await this.flow.callService(entity, command);
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = this._entityState();
    const playing = isMediaPlaying(state);
    const playback = getMediaPlayback(state);
    const vertical = Boolean(this._config.vertical);
    const name = String(
      this._config.name ?? state?.attributes?.friendly_name ?? entity ?? "媒体播放器",
    );
    const icon = String(this._config.icon ?? state?.attributes?.icon ?? "mdi:cast-audio");
    const tileColor = computeFeatureColor(playing, this._config.color, "primary");
    const colorStyle = styleMap({ "--tile-color": tileColor });
    const featureStyle = styleMap({
      "--feature-color": tileColor ?? (playing ? "var(--primary-color)" : "var(--state-inactive-color)"),
    });

    return html`
      <ha-card class=${classMap({ active: playing })} style=${colorStyle}>
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${playing}></ha-tile-icon>
          <ha-tile-info
            slot="info"
            .primary=${name}
            .secondary=${formatMediaSecondary(state)}
          ></ha-tile-info>
          <div slot="features" class="feature" style=${featureStyle}>
            <ha-control-media-playback
              .value=${playback}
              @value-changed=${this._onPlaybackChange}
              @click=${(ev: Event) => ev.stopPropagation()}
            ></ha-control-media-playback>
          </div>
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
        --feature-height: 40px;
        --feature-border-radius: var(--ha-card-border-radius, 12px);
      }
      ha-card {
        height: 100%;
        min-height: 0;
        display: flex;
        flex-direction: column;
        cursor: default;
        overflow: hidden;
      }
      ha-tile-container {
        flex: 1;
        min-height: 0;
      }
      ha-card:hover {
        box-shadow: var(--ha-card-box-shadow);
      }
      .feature {
        box-sizing: border-box;
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-media-card": HuiMediaCard;
  }
}
