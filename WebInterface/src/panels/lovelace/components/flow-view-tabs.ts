import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { mdiPencil, mdiPlus } from "@mdi/js";
import type { LovelaceViewConfig } from "../../../types";
import { viewUrlPath } from "../../../data/layout-mutations";
import "../../../components/ha-icon";

/**
 * Lightweight port of HA view tabs (hui-root + ha-tab-group).
 * Renders lovelace.views as a horizontal tab strip.
 */
@customElement("flow-view-tabs")
export class FlowViewTabs extends LitElement {
  @property({ attribute: false }) public views: LovelaceViewConfig[] = [];

  @property({ type: Number }) public activeIndex = 0;

  @property({ type: Boolean }) public editMode = false;

  private _select(index: number): void {
    if (index === this.activeIndex) return;
    this.dispatchEvent(
      new CustomEvent("view-selected", {
        bubbles: true,
        composed: true,
        detail: { index, path: viewUrlPath(this.views[index], index) },
      }),
    );
  }

  private _add(): void {
    this.dispatchEvent(
      new CustomEvent("view-add", {
        bubbles: true,
        composed: true,
      }),
    );
  }

  private _edit(index: number, ev: Event): void {
    ev.stopPropagation();
    this.dispatchEvent(
      new CustomEvent("view-edit", {
        bubbles: true,
        composed: true,
        detail: { index },
      }),
    );
  }

  private _delete(index: number, ev: Event): void {
    ev.stopPropagation();
    this.dispatchEvent(
      new CustomEvent("view-delete", {
        bubbles: true,
        composed: true,
        detail: { index },
      }),
    );
  }

  protected render() {
    if (!this.views.length) return nothing;

    return html`
      <div class="tabs" role="tablist">
        <div class="scroll">
          ${this.views.map((view, index) => {
            const active = index === this.activeIndex;
            const label = view.title || view.path || `视图 ${index + 1}`;
            const icon = view.icon || "mdi:view-dashboard";
            return html`
              <button
                type="button"
                role="tab"
                class="tab ${active ? "active" : ""}"
                aria-selected=${active ? "true" : "false"}
                title=${label}
                data-path=${viewUrlPath(view, index)}
                @click=${() => this._select(index)}
                @dblclick=${(ev: Event) => {
                  if (this.editMode) this._edit(index, ev);
                }}
              >
                <ha-icon class="tab-icon" .icon=${icon}></ha-icon>
                <span class="tab-label">${label}</span>
                ${this.editMode && active
                  ? html`
                      <span
                        class="tab-edit"
                        title="编辑视图"
                        role="button"
                        tabindex="0"
                        @click=${(ev: Event) => this._edit(index, ev)}
                        @keydown=${(ev: KeyboardEvent) => {
                          if (ev.key === "Enter" || ev.key === " ") {
                            ev.preventDefault();
                            this._edit(index, ev);
                          }
                        }}
                      >
                        <ha-svg-icon .path=${mdiPencil}></ha-svg-icon>
                      </span>
                    `
                  : nothing}
                ${this.editMode && this.views.length > 1
                  ? html`
                      <span
                        class="tab-delete"
                        title="删除视图"
                        role="button"
                        tabindex="0"
                        @click=${(ev: Event) => this._delete(index, ev)}
                        @keydown=${(ev: KeyboardEvent) => {
                          if (ev.key === "Enter" || ev.key === " ") {
                            ev.preventDefault();
                            this._delete(index, ev);
                          }
                        }}
                      >
                        ×
                      </span>
                    `
                  : nothing}
              </button>
            `;
          })}
        </div>
        ${this.editMode
          ? html`
              <button
                type="button"
                class="add"
                title="添加视图"
                aria-label="添加视图"
                @click=${this._add}
              >
                <ha-svg-icon .path=${mdiPlus}></ha-svg-icon>
              </button>
            `
          : nothing}
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      min-width: 0;
    }

    .tabs {
      display: flex;
      align-items: stretch;
      gap: 4px;
      min-width: 0;
      height: 100%;
    }

    .scroll {
      display: flex;
      align-items: stretch;
      gap: 2px;
      min-width: 0;
      overflow-x: auto;
      scrollbar-width: none;
    }

    .scroll::-webkit-scrollbar {
      display: none;
    }

    .tab {
      position: relative;
      display: inline-flex;
      align-items: center;
      gap: 8px;
      height: 100%;
      min-height: 40px;
      padding: 0 14px;
      border: none;
      border-bottom: 2px solid transparent;
      background: transparent;
      color: var(--app-header-text-color, var(--primary-text-color));
      opacity: 0.72;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
      font-weight: 500;
      white-space: nowrap;
      box-sizing: border-box;
      flex-shrink: 0;
    }

    .tab:hover {
      opacity: 1;
      background: rgba(0, 0, 0, 0.04);
    }

    .tab.active {
      opacity: 1;
      border-bottom-color: var(
        --ha-tab-indicator-color,
        var(--app-header-edit-text-color, var(--primary-color, #03a9f4))
      );
    }

    .tab-icon {
      --mdc-icon-size: 20px;
      width: 20px;
      height: 20px;
      flex-shrink: 0;
    }

    .tab-label {
      max-width: 160px;
      overflow: hidden;
      text-overflow: ellipsis;
    }

    .tab-edit,
    .tab-delete {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 22px;
      height: 22px;
      margin-left: 2px;
      border-radius: 50%;
      opacity: 0.65;
      flex-shrink: 0;
    }

    .tab-edit ha-svg-icon {
      width: 16px;
      height: 16px;
    }

    .tab-delete {
      font-size: 16px;
      line-height: 1;
    }

    .tab-edit:hover,
    .tab-delete:hover {
      opacity: 1;
      background: rgba(0, 0, 0, 0.12);
    }

    .tab-delete:hover {
      color: var(--error-color, #db4437);
    }

    .add {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 40px;
      height: 40px;
      margin: auto 0;
      border: none;
      border-radius: 50%;
      background: transparent;
      color: inherit;
      cursor: pointer;
      flex-shrink: 0;
      opacity: 0.85;
    }

    .add:hover {
      opacity: 1;
      background: rgba(0, 0, 0, 0.08);
    }

    .add ha-svg-icon {
      width: 22px;
      height: 22px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-view-tabs": FlowViewTabs;
  }
}
