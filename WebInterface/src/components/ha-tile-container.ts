import { css, html, LitElement } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";

/** Port of HA ha-tile-container */
@customElement("ha-tile-container")
export class HaTileContainer extends LitElement {
  @property({ type: Boolean }) public vertical = false;

  @property({ type: Boolean, attribute: "fixed-info-height" })
  public fixedInfoHeight = false;

  /**
   * When true, header stays content-sized and the features slot grows to fill
   * remaining card height (e.g. XY pad). Default HA behavior grows the header row.
   */
  @property({ type: Boolean, attribute: "expand-features", reflect: true })
  public expandFeatures = false;

  @state() private _hasFeatures = false;

  private _handleFeaturesSlotChange(ev: Event): void {
    this._hasFeatures = (ev.target as HTMLSlotElement).assignedElements().length > 0;
  }

  protected render() {
    const contentClasses = {
      content: true,
      vertical: this.vertical,
      "fixed-info-height": this.fixedInfoHeight,
      "has-features": this._hasFeatures,
    };

    return html`
      <div class="container">
        <div class="row">
          <div class=${classMap(contentClasses)}>
            <slot name="icon"></slot>
            <slot name="info"></slot>
          </div>
        </div>
        <slot name="features" @slotchange=${this._handleFeaturesSlotChange}></slot>
      </div>
    `;
  }

  static styles = css`
    :host {
      --ha-ripple-color: var(--tile-color);
      height: 100%;
      width: 100%;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      min-height: 0;
    }
    .container {
      display: flex;
      flex-direction: column;
      flex: 1;
      min-height: 0;
      margin: calc(-1 * var(--ha-card-border-width, 1px));
    }
    /* HA: column flex so .content can grow and vertically center */
    .row {
      display: flex;
      flex-direction: column;
      flex: 1;
      min-height: 0;
      min-width: 0;
    }
    :host([expand-features]) .row {
      flex: 0 0 auto;
      min-height: auto;
    }
    .content {
      position: relative;
      display: flex;
      flex-direction: row;
      align-items: center;
      padding: 0 10px;
      min-height: var(--row-height, 56px);
      flex: 1;
      min-width: 0;
      gap: 10px;
      box-sizing: border-box;
    }
    :host([expand-features]) .content {
      flex: 0 0 auto;
    }
    :host([expand-features]) .content.has-features:not(.vertical) {
      max-height: none;
    }
    .content.has-features:not(.vertical) {
      max-height: var(--row-height, 56px);
    }
    .content.vertical {
      flex-direction: column;
      text-align: center;
      justify-content: center;
      padding: 10px var(--ha-space-2, 8px);
      gap: 8px;
      max-height: none;
      --ha-tile-info-align-items: center;
      --ha-tile-info-text-align: center;
    }
    .content.vertical.fixed-info-height {
      gap: 2px;
      --ha-tile-info-gap: 2px;
      --ha-tile-info-primary-line-height: var(--ha-space-4, 16px);
      --ha-tile-info-primary-min-height: var(--ha-space-8, 32px);
      --ha-tile-info-min-height: var(--ha-space-12, 48px);
    }
    .content.vertical ::slotted([slot="info"]) {
      width: 100%;
      flex: none;
    }
    ::slotted([slot="icon"]) {
      flex-shrink: 0;
      position: relative;
      padding: 6px;
      margin: -6px;
    }
    ::slotted([slot="info"]) {
      position: relative;
      min-width: 0;
      flex: 1;
      box-sizing: border-box;
    }
    ::slotted([slot="features"]) {
      display: block;
      flex-shrink: 0;
      padding: 0 var(--ha-space-3, 12px) var(--ha-space-3, 12px);
      box-sizing: border-box;
    }
    :host([expand-features]) ::slotted([slot="features"]) {
      flex: 1 1 auto;
      flex-shrink: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-tile-container": HaTileContainer;
  }
}
