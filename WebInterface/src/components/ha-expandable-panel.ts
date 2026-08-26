import { css, html, LitElement } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import type { PropertyValues } from "lit";

@customElement("ha-expandable-panel")
export class HaExpandablePanel extends LitElement {
  @property() public header = "";

  @property({ type: Boolean, reflect: true }) public expanded = true;

  @state() private _open = true;

  protected willUpdate(changed: PropertyValues): void {
    // Only sync from the expanded prop when it changes — otherwise every
    // parent re-render would force the panel open and block collapsing.
    if (changed.has("expanded")) {
      this._open = this.expanded;
    }
  }

  private _toggle(): void {
    this._open = !this._open;
  }

  protected render() {
    return html`
      <div class="panel">
        <button type="button" class="header" @click=${this._toggle}>
          <span class="chevron ${classMap({ open: this._open })}">›</span>
          <span class="title">${this.header}</span>
        </button>
        ${this._open
          ? html`<div class="body"><slot></slot></div>`
          : ""}
      </div>
    `;
  }

  static styles = css`
    .panel {
      border: 1px solid var(--divider-color);
      border-radius: 12px;
      margin-bottom: 12px;
      background: var(--card-background-color, #fff);
    }
    .header {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 12px 14px;
      border: none;
      background: transparent;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
      font-weight: 600;
      color: var(--primary-text-color);
      text-align: left;
    }
    .chevron {
      display: inline-block;
      transition: transform 180ms ease;
      color: var(--secondary-text-color);
      font-size: 18px;
      line-height: 1;
    }
    .chevron.open {
      transform: rotate(90deg);
    }
    .body {
      padding: 0 14px 14px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-expandable-panel": HaExpandablePanel;
  }
}
