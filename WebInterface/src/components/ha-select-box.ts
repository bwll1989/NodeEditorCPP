import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";

export interface SelectBoxOption {
  value: string;
  label: string;
  description?: string;
  /** Inline SVG / img URL for the preview graphic */
  image?: string;
  disabled?: boolean;
}

/** Simplified port of HA `ha-select-box` (select mode: box). */
@customElement("ha-select-box")
export class HaSelectBox extends LitElement {
  @property({ attribute: false }) public options: SelectBoxOption[] = [];

  @property() public value?: string;

  @property() public label?: string;

  @property({ type: Boolean }) public disabled = false;

  @property({ type: Number, attribute: "max_columns" })
  public maxColumns = 3;

  private _select(value: string): void {
    if (this.disabled || value === (this.value ?? "")) return;
    this.dispatchEvent(
      new CustomEvent("value-changed", {
        detail: { value },
        bubbles: true,
        composed: true,
      }),
    );
  }

  protected render() {
    const columns = Math.min(this.maxColumns, Math.max(this.options.length, 1));

    return html`
      ${this.label ? html`<div class="label">${this.label}</div>` : nothing}
      <div class="list" style="--columns: ${columns}">
        ${this.options.map((option) => {
          const selected = option.value === this.value;
          const disabled = Boolean(option.disabled || this.disabled);
          return html`
            <button
              type="button"
              class=${classMap({ option: true, selected })}
              ?disabled=${disabled}
              @click=${() => this._select(option.value)}
            >
              <div class="content">
                <span class=${classMap({ radio: true, checked: selected })}></span>
                <div class="text">
                  <span class="option-label">${option.label}</span>
                  ${option.description
                    ? html`<span class="description">${option.description}</span>`
                    : nothing}
                </div>
              </div>
              ${option.image
                ? html`<img class="preview" src=${option.image} alt="" draggable="false" />`
                : nothing}
            </button>
          `;
        })}
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      margin-bottom: 12px;
    }
    .label {
      display: block;
      margin: 0 0 8px;
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .list {
      display: grid;
      grid-template-columns: repeat(var(--columns, 1), minmax(0, 1fr));
      gap: 12px;
    }
    .option {
      position: relative;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      padding: 12px;
      border: 1px solid var(--divider-color);
      border-radius: 12px;
      background: #fff;
      overflow: hidden;
      cursor: pointer;
      font: inherit;
      color: inherit;
      text-align: left;
      min-height: 0;
    }
    .option::before {
      content: "";
      position: absolute;
      inset: 0;
      background: transparent;
      opacity: 0.2;
      pointer-events: none;
      transition:
        background-color 180ms ease-in-out,
        opacity 180ms ease-in-out;
    }
    .option:hover:not(:disabled)::before {
      background-color: var(--divider-color);
    }
    .option.selected::before {
      background-color: var(--primary-color);
    }
    .option:disabled {
      cursor: not-allowed;
      opacity: 0.55;
    }
    .content {
      position: relative;
      display: flex;
      flex-direction: row;
      align-items: center;
      gap: 8px;
      width: 100%;
      min-width: 0;
      z-index: 1;
    }
    .radio {
      flex: none;
      width: 18px;
      height: 18px;
      border: 2px solid var(--secondary-text-color);
      border-radius: 50%;
      box-sizing: border-box;
      position: relative;
    }
    .radio.checked {
      border-color: var(--primary-color);
    }
    .radio.checked::after {
      content: "";
      position: absolute;
      inset: 3px;
      border-radius: 50%;
      background: var(--primary-color);
    }
    .text {
      display: flex;
      flex-direction: column;
      gap: 2px;
      min-width: 0;
      flex: 1;
    }
    .option-label {
      color: var(--primary-text-color);
      font-size: 14px;
      line-height: 1.25;
      overflow: hidden;
      white-space: nowrap;
      text-overflow: ellipsis;
    }
    .description {
      color: var(--secondary-text-color);
      font-size: 12px;
      line-height: 1.25;
    }
    .preview {
      position: relative;
      z-index: 1;
      max-width: 96px;
      max-height: 96px;
      margin: auto;
      pointer-events: none;
      user-select: none;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-select-box": HaSelectBox;
  }
}
