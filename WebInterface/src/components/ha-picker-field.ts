import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";

/**
 * HA-style picker trigger: leading icon, overline label + value, trailing chevron.
 * Matches home-assistant/frontend ha-picker-field layout (56px height).
 */
@customElement("ha-picker-field")
export class HaPickerField extends LitElement {
  @property() public label = "";

  @property({ type: Boolean }) public hasValue = false;

  @property({ type: Boolean }) public disabled = false;

  @property({ type: Boolean, reflect: true }) public opened = false;

  @property({ type: Boolean, reflect: true }) public compact = false;

  protected render() {
    const overline = this.hasValue && this.label
      ? html`<span class="overline">${this.label}</span>`
      : nothing;

    const headline = this.hasValue
      ? html`<span class="headline"><slot name="value"></slot></span>`
      : html`<span class="headline placeholder">${this.label}</span>`;

    return html`
      <button
        type="button"
        class="field ${this.compact ? "compact" : ""}"
        ?disabled=${this.disabled}
        part="button"
      >
        <span class="leading"><slot name="leading"></slot></span>
        <span class="text">
          ${overline}
          ${headline}
        </span>
        <span class="trailing" aria-hidden="true">
          <svg viewBox="0 0 24 24" class="chevron">
            <path d="M7 10l5 5 5-5z"></path>
          </svg>
        </span>
      </button>
    `;
  }

  static styles = css`
    :host {
      display: block;
      min-width: 0;
    }
    .field {
      width: 100%;
      min-height: 56px;
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 0 8px 0 12px;
      border: none;
      border-radius: 4px 4px 0 0;
      background: var(--ha-color-form-background, #f5f5f5);
      cursor: pointer;
      font: inherit;
      text-align: left;
      color: var(--primary-text-color);
      position: relative;
      box-sizing: border-box;
    }
    .field:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .field.compact {
      min-height: 36px;
      padding: 0 6px 0 8px;
      border-radius: 8px;
    }
    .field.compact .text {
      padding: 4px 0;
    }
    .field.compact .overline {
      display: none;
    }
    .field.compact .headline {
      font-size: 13px;
    }
    .field.compact .trailing {
      flex-basis: 24px;
    }
    .field.compact .chevron {
      width: 18px;
      height: 18px;
    }
    .field::after {
      content: "";
      position: absolute;
      left: 0;
      right: 0;
      bottom: 0;
      height: 1px;
      background: var(--ha-color-border-neutral-loud, var(--divider-color, #ccc));
      transition:
        height 180ms ease,
        background-color 180ms ease;
    }
    :host([opened]) .field::after {
      height: 2px;
      background: var(--primary-color);
    }
    .leading {
      flex: 0 0 auto;
      display: flex;
      align-items: center;
      justify-content: center;
      width: 24px;
      height: 24px;
    }
    .text {
      flex: 1;
      min-width: 0;
      display: flex;
      flex-direction: column;
      justify-content: center;
      gap: 2px;
      padding: 8px 0;
    }
    .overline {
      font-size: 12px;
      line-height: 1.2;
      color: var(--secondary-text-color);
    }
    .headline {
      font-size: 14px;
      line-height: 1.3;
      font-weight: 400;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .headline.placeholder {
      color: var(--secondary-text-color);
    }
    .trailing {
      flex: 0 0 32px;
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--secondary-text-color);
    }
    .chevron {
      width: 20px;
      height: 20px;
      fill: currentColor;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-picker-field": HaPickerField;
  }
}
