import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-card";

function linkSubtitle(url: string): string {
  const text = url.trim();
  if (!text) return "未设置链接";
  try {
    const parsed = new URL(text.includes("://") ? text : `https://${text}`);
    return parsed.hostname + (parsed.pathname !== "/" ? parsed.pathname : "");
  } catch {
    return text.length > 48 ? `${text.slice(0, 45)}…` : text;
  }
}

function normalizeUrl(url: string): string {
  const text = url.trim();
  if (!text) return "";
  if (/^[a-z][a-z0-9+.-]*:/i.test(text)) return text;
  return `https://${text}`;
}

@customElement("hui-link-card")
export class HuiLinkCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "link",
      name: "超链接",
      url: "http://127.0.0.1:8992/",
      icon: "mdi:link-variant",
      new_tab: true,
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    return {
      columns: 6,
      rows: 1,
      min_columns: 3,
      min_rows: 1,
    };
  }

  protected render() {
    if (!this._config) return nothing;

    const name = String(this._config.name ?? "超链接");
    const rawUrl = String(this._config.url ?? "");
    const url = normalizeUrl(rawUrl);
    const icon = String(this._config.icon ?? "mdi:link-variant");
    const newTab = this._config.new_tab !== false;
    const disabled = !url;

    return html`
      <ha-card class=${disabled ? "disabled" : ""}>
        ${disabled
          ? html`
              <div class="link-body static">
                <ha-tile-container>
                  <ha-tile-icon slot="icon" .icon=${icon}></ha-tile-icon>
                  <ha-tile-info slot="info" .primary=${name} .secondary=${linkSubtitle(rawUrl)}></ha-tile-info>
                </ha-tile-container>
              </div>
            `
          : html`
              <a
                class="link-body"
                href=${url}
                target=${newTab ? "_blank" : "_self"}
                rel=${newTab ? "noopener noreferrer" : nothing}
                @click=${(ev: Event) => ev.stopPropagation()}
              >
                <ha-tile-container>
                  <ha-tile-icon slot="icon" .icon=${icon} .active=${true}></ha-tile-icon>
                  <ha-tile-info slot="info" .primary=${name} .secondary=${linkSubtitle(rawUrl)}></ha-tile-info>
                </ha-tile-container>
              </a>
            `}
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
        padding: 0;
        overflow: hidden;
      }
      ha-card.disabled {
        opacity: 0.72;
      }
      .link-body {
        display: block;
        height: 100%;
        min-height: 0;
        color: inherit;
        text-decoration: none;
        box-sizing: border-box;
      }
      .link-body.static {
        cursor: default;
      }
      a.link-body {
        cursor: pointer;
      }
      a.link-body:hover ha-card,
      a.link-body:focus-visible {
        outline: none;
      }
      a.link-body:hover {
        filter: brightness(0.98);
      }
      ha-tile-container {
        height: 100%;
        min-height: 0;
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-link-card": HuiLinkCard;
  }
}
