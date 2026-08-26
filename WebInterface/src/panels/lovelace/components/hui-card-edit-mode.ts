import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { fireEvent } from "../../../common/dom/fire_event";
import { onEditMenuOpen } from "../../../common/dom/edit-menu-bus";
import "../../../components/ha-action-menu";
import "../../../components/ha-icon";
import type { ActionMenuItem } from "../../../components/ha-action-menu";

export interface CardEditPath {
  sectionIndex: number;
  cardIndex: number;
}

/** Port of HA hui-card-edit-mode (simplified) */
@customElement("hui-card-edit-mode")
export class HuiCardEditMode extends LitElement {
  @property({ type: Number }) public sectionIndex = 0;

  @property({ type: Number }) public cardIndex = 0;

  @property({ type: Boolean, attribute: "no-edit" }) public noEdit = false;

  @property({ type: Boolean, attribute: "hidden-overlay" }) public hiddenOverlay = false;

  @property({ type: Boolean }) public compact = false;

  /** When true, edit overlay is hidden but the slotted card stays mounted. */
  @property({ type: Boolean, reflect: true }) public inactive = false;

  @state() private _hover = false;

  @state() private _focused = false;

  @state() private _menuOpen = false;

  private _touchStarted = false;

  private _unsubMenuOpen?: () => void;

  connectedCallback(): void {
    super.connectedCallback();
    this._unsubMenuOpen = onEditMenuOpen((source) => {
      if (!this.contains(source)) {
        this._hover = false;
        this._focused = false;
        this._menuOpen = false;
      }
    });
    this.addEventListener("focus", () => {
      this._focused = true;
    });
    this.addEventListener("blur", () => {
      this._focused = false;
    });
    this.addEventListener("touchstart", () => {
      this._touchStarted = true;
      this._hover = true;
      document.addEventListener("click", this._documentClicked);
    });
    this.addEventListener("touchend", () => {
      setTimeout(() => {
        this._touchStarted = false;
      }, 10);
    });
    this.addEventListener("mouseenter", () => {
      if (this._touchStarted) return;
      this._hover = true;
    });
    this.addEventListener("mouseleave", () => {
      if (this._menuOpen) return;
      this._hover = false;
    });
    this.addEventListener("click", () => {
      this._hover = true;
      document.addEventListener("click", this._documentClicked);
    });
  }

  disconnectedCallback(): void {
    document.removeEventListener("click", this._documentClicked);
    this._unsubMenuOpen?.();
    super.disconnectedCallback();
  }

  private _documentClicked = (ev: MouseEvent): void => {
    this._hover = ev.composedPath().includes(this);
    document.removeEventListener("click", this._documentClicked);
  };

  protected updated(changed: PropertyValues): void {
    if (changed.has("inactive") && this.inactive) {
      this._hover = false;
      this._focused = false;
      this._menuOpen = false;
      this.toggleAttribute("menu-open", false);
    }
  }

  private _path(): CardEditPath {
    return { sectionIndex: this.sectionIndex, cardIndex: this.cardIndex };
  }

  private _menuItems(): ActionMenuItem[] {
    const items: ActionMenuItem[] = [];
    if (!this.noEdit) {
      items.push({ value: "edit", label: "编辑", icon: "mdi:pencil" });
    }
    items.push({ value: "duplicate", label: "创建副本", icon: "mdi:plus-circle-multiple-outline" });
    items.push({ value: "copy", label: "复制", icon: "mdi:content-copy" });
    items.push({ value: "cut", label: "剪切", icon: "mdi:content-cut" });
    if (items.length) {
      items.push({ value: "divider", label: "", divider: true });
    }
    items.push({ value: "delete", label: "删除", icon: "mdi:delete", destructive: true });
    return items;
  }

  private _onMenuAction(ev: CustomEvent<{ value: string }>): void {
    const path = this._path();
    switch (ev.detail.value) {
      case "edit":
        fireEvent(this, "ll-edit-card", path);
        break;
      case "duplicate":
        fireEvent(this, "ll-duplicate-card", path);
        break;
      case "copy":
        fireEvent(this, "ll-copy-card", path);
        break;
      case "cut":
        fireEvent(this, "ll-copy-card", path);
        fireEvent(this, "ll-delete-card", { ...path, silent: true });
        break;
      case "delete":
        fireEvent(this, "ll-delete-card", { ...path, silent: false });
        break;
      default:
        break;
    }
  }

  private _onMenuOpened(): void {
    this._menuOpen = true;
    this.toggleAttribute("menu-open", true);
    this._hover = true;
  }

  private _onMenuClosed(): void {
    this._menuOpen = false;
    this.toggleAttribute("menu-open", false);
  }

  private _editOverlay(ev: Event): void {
    if (this.noEdit) return;
    ev.preventDefault();
    ev.stopPropagation();
    fireEvent(this, "ll-edit-card", this._path());
  }

  protected render() {
    if (this.inactive) {
      return html`
        <div class="card-wrapper">
          <slot></slot>
        </div>
      `;
    }

    const overlayHidden = this.hiddenOverlay || this.compact;
    const showOverlay = (this._hover || this._focused || this._menuOpen) && !overlayHidden;
    const showMenu = this._hover || this._focused || this._menuOpen;
    const elevated = this._hover || this._focused || this._menuOpen;

    return html`
      <div class="card-wrapper ${elevated ? "elevated" : ""}">
        <slot></slot>
        <div
          class=${classMap({
            "card-overlay": true,
            visible: showOverlay,
            compact: overlayHidden,
            "menu-visible": showMenu,
          })}
        >
          ${overlayHidden || this.noEdit
            ? nothing
            : html`
                <button type="button" class="control" @click=${this._editOverlay} title="编辑">
                  <div class="control-overlay"></div>
                  <ha-icon class="control-icon" icon="mdi:pencil"></ha-icon>
                </button>
              `}
          ${this.noEdit && !overlayHidden
            ? html`
                <div class="control drag-hint" title="拖动以调整位置">
                  <div class="control-overlay"></div>
                  <ha-icon class="control-icon" icon="mdi:cursor-move"></ha-icon>
                </div>
              `
            : nothing}
          <ha-action-menu
            class="more ${showMenu ? "" : "hidden"}"
            .items=${this._menuItems()}
            @action=${this._onMenuAction}
            @menu-opened=${this._onMenuOpened}
            @menu-closed=${this._onMenuClosed}
          >
            <ha-icon slot="trigger" icon="mdi:dots-vertical"></ha-icon>
          </ha-action-menu>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
      position: relative;
      z-index: 0;
    }
    :host(:hover),
    :host(:focus-within) {
      z-index: 2;
    }
    :host([menu-open]) {
      z-index: 5;
    }
    .card-wrapper {
      position: relative;
      height: 100%;
      z-index: 0;
    }
    .card-wrapper.elevated {
      z-index: 2;
    }
    .card-overlay {
      position: absolute;
      inset: 0;
      opacity: 0;
      pointer-events: none;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: opacity 180ms ease-in-out;
      z-index: 2;
    }
    .card-overlay.visible {
      opacity: 1;
      pointer-events: auto;
    }
    .card-overlay.compact {
      align-items: flex-start;
      justify-content: flex-end;
    }
    .card-overlay.compact.visible,
    .card-overlay.compact.menu-visible {
      opacity: 1;
      pointer-events: none;
    }
    .control {
      outline: none;
      cursor: pointer;
      position: absolute;
      inset: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      border: none;
      background: transparent;
      border-radius: var(--ha-card-border-radius, 12px);
      padding: 0;
    }
    .control-overlay {
      position: absolute;
      inset: 0;
      opacity: 0.8;
      background-color: var(--primary-background-color);
      border: 1px solid var(--divider-color);
      border-radius: var(--ha-card-border-radius, 12px);
    }
    .control-icon {
      position: relative;
      z-index: 1;
      color: var(--primary-text-color);
      border-radius: var(--ha-border-radius-circle, 50%);
      padding: 8px;
      background-color: var(--secondary-background-color);
      --mdc-icon-size: 20px;
    }
    .more {
      position: absolute;
      right: -6px;
      top: -6px;
      z-index: 3;
      pointer-events: auto;
      --ha-action-menu-trigger-background: var(--secondary-background-color);
      --ha-action-menu-trigger-hover-background: color-mix(
        in srgb,
        var(--secondary-background-color) 88%,
        #000000
      );
      --ha-action-menu-trigger-size: 32px;
      --ha-action-menu-trigger-radius: 50%;
    }
    .more.hidden {
      opacity: 0;
      pointer-events: none;
    }
    .card-overlay.compact .more {
      right: 0;
      top: 0;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-card-edit-mode": HuiCardEditMode;
  }
}
