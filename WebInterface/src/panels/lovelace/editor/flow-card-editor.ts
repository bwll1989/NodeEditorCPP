import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { consume } from "@lit/context";
import type {
  Flow,
  LovelaceCardConfig,
  LovelaceSectionConfig,
} from "../../../types";
import { flowContext } from "../../../context/flow-context";
import { createBackdropDismissHandlers } from "../../../common/dom/modal-backdrop";
import "../flow-section";
import "./flow-card-config-editor";
import "./flow-card-layout-editor";

type EditorTab = "config" | "layout";

/**
 * Port of HA `hui-dialog-edit-card` preview:
 * render the card inside a real section grid (not stretched to fill the pane).
 */
@customElement("flow-card-editor")
export class FlowCardEditor extends LitElement {
  @consume({ context: flowContext, subscribe: true })
  @property({ attribute: false })
  public flow?: Flow;

  @property({ type: Boolean }) public open = false;

  @property({ attribute: false }) public config?: LovelaceCardConfig;

  /** Parent section config (HA `_sectionConfig`) — used for accurate grid preview */
  @property({ attribute: false }) public sectionConfig?: LovelaceSectionConfig;

  @state() private _draft: LovelaceCardConfig = { type: "tile" };

  @state() private _tab: EditorTab = "config";

  private _layoutRevision = 0;

  private readonly _backdropDismiss = createBackdropDismissHandlers(() => this._close());

  protected updated(changed: import("lit").PropertyValues): void {
    if (changed.has("open") && this.open) {
      this._tab = "config";
    }
    if (changed.has("config") && this.config) {
      this._draft = structuredClone(this.config);
      this._layoutRevision += 1;
    }
  }

  private _close(): void {
    this.dispatchEvent(new CustomEvent("close", { bubbles: true, composed: true }));
  }

  private _save(): void {
    this.dispatchEvent(
      new CustomEvent("save", {
        bubbles: true,
        composed: true,
        detail: { config: structuredClone(this._draft) },
      }),
    );
  }

  private _onConfigChanged(ev: CustomEvent<{ config: LovelaceCardConfig }>): void {
    ev.stopPropagation();
    this._draft = ev.detail.config;
    this._layoutRevision += 1;
  }

  private _onPicturePosition(ev: CustomEvent<{ left: number; top: number }>): void {
    ev.stopPropagation();
    if (this._tab !== "config") return;
    const editor = this.renderRoot.querySelector("flow-card-config-editor") as
      | import("./flow-card-config-editor").FlowCardConfigEditor
      | null;
    editor?.applyPictureElementPosition(ev.detail.left, ev.detail.top);
  }

  private _onPictureElementsChanged(ev: CustomEvent<{ config: LovelaceCardConfig }>): void {
    ev.stopPropagation();
    this._draft = ev.detail.config;
    this._layoutRevision += 1;
  }

  /** HA `_cardConfigInSection` — section shell with only the edited card */
  private _previewSectionConfig(): LovelaceSectionConfig {
    const section = this.sectionConfig;
    if (section) {
      const { cards: _cards, title: _title, ...containerConfig } = section;
      return {
        ...containerConfig,
        type: section.type ?? "grid",
        // Preview at one section-column width (HA dialog max-width ≈ 500px)
        column_span: 1,
        cards: [this._draft],
      };
    }
    return {
      type: "grid",
      column_span: 1,
      cards: [this._draft],
    };
  }

  private _cardTypeName(): string {
    const names: Record<string, string> = {
      tile: "Toggle",
      heading: "Title",
      sensor: "数值",
      switch: "开关",
      trigger: "触发器",
      climate: "步进",
      cover: "单选",
      clock: "Clock",
      slider: "滑块",
      gain: "增益",
      media: "媒体",
      markdown: "Markdown",
      link: "超链接",
      "picture-elements": "图片元素",
      line2d: "2D 折线",
      scatter2d: "2D 散点",
      bar: "柱状图",
      line3d: "3D 折线",
      scatter3d: "3D 散点",
    };
    return names[String(this._draft.type)] ?? String(this._draft.type);
  }

  protected render() {
    if (!this.open) return nothing;

    const previewSection = this._previewSectionConfig();

    return html`
      <div
        class="backdrop"
        @pointerdown=${this._backdropDismiss.onBackdropPointerDown}
        @pointerup=${this._backdropDismiss.onBackdropPointerUp}
        @pointercancel=${this._backdropDismiss.onBackdropPointerCancel}
      >
        <div class="dialog">
          <header class="header">
            <h3>${this._cardTypeName()} 卡片配置</h3>
            <button type="button" class="close" @click=${this._close} aria-label="关闭">×</button>
          </header>

          <nav class="tabs">
            <button
              type="button"
              class="tab ${this._tab === "config" ? "active" : ""}"
              @click=${() => {
                this._tab = "config";
              }}
            >
              配置
            </button>
            <button
              type="button"
              class="tab ${this._tab === "layout" ? "active" : ""}"
              @click=${() => {
                this._tab = "layout";
              }}
            >
              布局
            </button>
          </nav>

          <div class="body">
            <div class="editor-pane">
              ${this._tab === "config"
                ? html`
                    <flow-card-config-editor
                      .config=${this._draft}
                      @config-changed=${this._onConfigChanged}
                    ></flow-card-config-editor>
                  `
                : html`
                    <flow-card-layout-editor
                      .config=${this._draft}
                      .sectionConfig=${this.sectionConfig}
                      @config-changed=${this._onConfigChanged}
                    ></flow-card-layout-editor>
                  `}
            </div>

            <div
              class="preview-pane element-preview"
              @picture-element-position=${this._onPicturePosition}
              @picture-elements-changed=${this._onPictureElementsChanged}
            >
              <div class="preview-label">预览</div>
              <flow-section
                class="preview-section"
                style="--column-span: 1;"
                .config=${previewSection}
                .editMode=${false}
                .preview=${true}
                .layoutRevision=${this._layoutRevision}
              ></flow-section>
            </div>
          </div>

          <div class="actions">
            <button type="button" class="secondary" @click=${this._close}>取消</button>
            <button type="button" class="primary" @click=${this._save}>保存</button>
          </div>
        </div>
      </div>
    `;
  }

  static styles = css`
    .backdrop {
      position: fixed;
      inset: 0;
      background: rgba(15, 23, 42, 0.45);
      display: flex;
      align-items: center;
      justify-content: center;
      z-index: 200;
      padding: 16px;
      box-sizing: border-box;
    }
    .dialog {
      width: min(920px, 100%);
      max-height: min(90vh, 720px);
      background: var(--card-background-color);
      border-radius: 16px;
      display: flex;
      flex-direction: column;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.18);
      overflow: hidden;
    }
    .header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 16px 20px 0;
    }
    h3 {
      margin: 0;
      font-size: 18px;
      font-weight: 600;
    }
    .close {
      border: none;
      background: transparent;
      font-size: 24px;
      line-height: 1;
      cursor: pointer;
      color: var(--secondary-text-color);
      padding: 4px 8px;
    }
    .tabs {
      display: flex;
      gap: 0;
      padding: 0 20px;
      border-bottom: 1px solid var(--divider-color);
      margin-top: 12px;
    }
    .tab {
      border: none;
      background: transparent;
      padding: 12px 16px;
      font-size: 14px;
      font-weight: 500;
      color: var(--secondary-text-color);
      cursor: pointer;
      border-bottom: 2px solid transparent;
      margin-bottom: -1px;
    }
    .tab.active {
      color: var(--primary-color);
      border-bottom-color: var(--primary-color);
    }
    .body {
      display: flex;
      flex: 1;
      min-height: 0;
      overflow: hidden;
    }
    .editor-pane {
      flex: 1;
      padding: 20px;
      overflow: auto;
      min-width: 0;
    }
    /* HA .element-preview */
    .preview-pane {
      flex: 0 0 min(320px, 40%);
      border-left: 1px solid var(--divider-color);
      background: var(--primary-background-color);
      padding: 12px;
      display: flex;
      flex-direction: column;
      gap: 8px;
      overflow: auto;
      min-width: 0;
      box-sizing: border-box;
    }
    .preview-label {
      font-size: 12px;
      font-weight: 600;
      color: var(--secondary-text-color);
      text-transform: uppercase;
      letter-spacing: 0.04em;
      flex: none;
    }
    /*
      HA: .content hui-section {
        max-width: var(--ha-view-sections-column-max-width, 500px);
        margin: 0 auto;
      }
      Card keeps its grid_options size inside the section — no stretch fill.
    */
    .preview-section {
      display: block;
      width: 100%;
      max-width: var(--ha-view-sections-column-max-width, 500px);
      margin: 0 auto;
      padding: 4px;
      box-sizing: border-box;
    }
    .actions {
      display: flex;
      justify-content: flex-end;
      gap: 8px;
      padding: 12px 20px 16px;
      border-top: 1px solid var(--divider-color);
    }
    button {
      border-radius: 8px;
      padding: 8px 14px;
      font-weight: 600;
      cursor: pointer;
      border: 1px solid var(--divider-color);
      background: #fff;
    }
    button.primary {
      background: var(--primary-color);
      border-color: var(--primary-color);
      color: #fff;
    }
    @media (min-width: 1000px) {
      .dialog {
        width: min(1100px, 100%);
      }
      .preview-pane {
        flex: 1 1 0;
        max-width: 520px;
      }
      .preview-section {
        padding: 8px 10px;
      }
    }
    @media (max-width: 720px) {
      .body {
        flex-direction: column;
      }
      .preview-pane {
        flex: none;
        max-width: none;
        border-left: none;
        border-top: 1px solid var(--divider-color);
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-card-editor": FlowCardEditor;
  }
}
