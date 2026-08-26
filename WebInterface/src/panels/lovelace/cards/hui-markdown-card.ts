import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import "../../../components/ha-card";
import "../../../components/ha-markdown";

@customElement("hui-markdown-card")
export class HuiMarkdownCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "markdown",
      content:
        "The **Markdown** card allows you to write any text. You can style it **bold**, *italicized*, ~~strikethrough~~ etc. You can do images, links, and more.\n\nFor more information see the [Markdown Cheatsheet](https://commonmark.org/help).",
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  setConfig(config: LovelaceCardConfig): void {
    if (!config.content && config.content !== "") {
      throw new Error("Content required");
    }
    this._config = config;
  }

  getGridOptions(): LovelaceGridOptions {
    return { columns: "full", rows: "auto", min_columns: 12, min_rows: 1 };
  }

  protected render() {
    if (!this._config) return nothing;

    const textOnly = Boolean(this._config.text_only);
    const title = String(this._config.title ?? "");
    const content = String(this._config.content ?? "");

    return html`
      <ha-card
        class=${classMap({
          "text-only": textOnly,
          "with-header": Boolean(title) && !textOnly,
        })}
      >
        ${title && !textOnly ? html`<h1 class="card-header">${title}</h1>` : nothing}
        <ha-markdown
          breaks
          .content=${content}
        ></ha-markdown>
      </ha-card>
    `;
  }

  static styles = css`
    ha-card {
      height: 100%;
      overflow-y: auto;
    }
    .card-header {
      margin: 0;
      padding: 16px 16px 0;
      font-size: 16px;
      font-weight: 600;
      color: var(--primary-text-color);
      line-height: 1.3;
    }
    ha-markdown {
      display: block;
      padding: 16px;
      word-wrap: break-word;
      overflow-wrap: anywhere;
    }
    .with-header ha-markdown {
      padding-top: 8px;
    }
    .text-only {
      background: none;
      box-shadow: none;
      border: none;
    }
    .text-only ha-markdown {
      padding: 2px 4px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-markdown-card": HuiMarkdownCard;
  }
}
