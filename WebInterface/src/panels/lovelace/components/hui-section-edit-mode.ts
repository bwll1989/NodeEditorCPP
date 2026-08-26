import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import { fireEvent } from "../../../common/dom/fire_event";
import { onEditMenuOpen } from "../../../common/dom/edit-menu-bus";
import "../../../components/ha-action-menu";
import type { ActionMenuItem } from "../../../components/ha-action-menu";

@customElement("hui-section-edit-mode")
export class HuiSectionEditMode extends LitElement {
  @property({ type: Number }) public sectionIndex = 0;

  /** When true, section chrome is hidden but slotted content stays mounted. */
  @property({ type: Boolean, reflect: true }) public inactive = false;

  private _unsubMenuOpen?: () => void;

  connectedCallback(): void {
    super.connectedCallback();
    this._unsubMenuOpen = onEditMenuOpen((source) => {
      if (!this.contains(source)) {
        this.toggleAttribute("menu-open", false);
      }
    });
  }

  disconnectedCallback(): void {
    this._unsubMenuOpen?.();
    super.disconnectedCallback();
  }

  private _path(): { sectionIndex: number } {
    return { sectionIndex: this.sectionIndex };
  }

  private _menuItems(): ActionMenuItem[] {
    return [
      { value: "edit", label: "编辑", icon: "mdi:pencil" },
      { value: "duplicate", label: "复制", icon: "mdi:content-copy" },
      { value: "divider", label: "", divider: true },
      { value: "delete", label: "删除", icon: "mdi:delete", destructive: true },
    ];
  }

  private _onMenuAction(ev: CustomEvent<{ value: string }>): void {
    const path = this._path();
    switch (ev.detail.value) {
      case "edit":
        fireEvent(this, "ll-edit-section", path);
        break;
      case "duplicate":
        fireEvent(this, "ll-duplicate-section", path);
        break;
      case "delete":
        fireEvent(this, "ll-delete-section", path);
        break;
      default:
        break;
    }
  }

  protected render() {
    return html`
      <div class="section-header">
        <div class="section-actions">
          <span class="section-handle handle" title="拖动以调整分区位置">≡</span>
          <ha-action-menu
            .items=${this._menuItems()}
            @action=${this._onMenuAction}
            @menu-opened=${() => {
              this.toggleAttribute("menu-open", true);
            }}
            @menu-closed=${() => {
              this.toggleAttribute("menu-open", false);
            }}
          ></ha-action-menu>
        </div>
      </div>
      <div class="section-wrapper">
        <slot></slot>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      position: relative;
      z-index: 10;
    }
    :host([menu-open]) {
      z-index: 20;
    }
    :host([inactive]) {
      z-index: auto;
    }
    :host([inactive]) .section-header {
      display: none;
    }
    :host([inactive]) .section-wrapper {
      padding: 0;
      border: none;
      min-height: 0;
    }
    .section-header {
      position: relative;
      height: 34px;
      display: flex;
      flex-direction: column;
      justify-content: flex-end;
    }
    .section-actions {
      position: absolute;
      height: 36px;
      bottom: -2px;
      right: 0;
      inset-inline-end: 0;
      inset-inline-start: initial;
      display: flex;
      align-items: center;
      justify-content: center;
      border-radius: var(--ha-section-border-radius, 16px);
      border-bottom-left-radius: 0;
      border-bottom-right-radius: 0;
      background-color: var(--secondary-background-color);
      color: var(--primary-text-color);
      --ha-icon-button-size: 36px;
      --mdc-icon-size: 20px;
      z-index: 2;
    }
    .handle {
      cursor: grab;
      padding: 8px;
      user-select: none;
      font-size: 18px;
      line-height: 1;
    }
    .handle:active {
      cursor: grabbing;
    }
    ha-action-menu {
      display: inline-flex;
      --ha-action-menu-trigger-background: transparent;
      --ha-action-menu-trigger-hover-background: rgba(0, 0, 0, 0.04);
      --ha-action-menu-trigger-size: 36px;
      --ha-action-menu-trigger-radius: 0 var(--ha-section-border-radius, 16px) 0 0;
    }
    .section-wrapper {
      padding: var(--ha-space-2, 8px);
      border-radius: var(--ha-section-border-radius, 16px);
      border-start-end-radius: 0;
      border: 2px dashed var(--divider-color);
      min-height: var(--row-height, var(--ha-view-sections-row-height, 56px));
      box-sizing: border-box;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-section-edit-mode": HuiSectionEditMode;
  }
}
