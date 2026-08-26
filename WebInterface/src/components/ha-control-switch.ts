import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import "./ha-icon";

/** Simplified port of HA ha-control-switch (horizontal toggle feature). */
@customElement("ha-control-switch")
export class HaControlSwitch extends LitElement {
  @property({ type: Boolean }) public checked = false;

  @property({ type: Boolean, reflect: true }) public disabled = false;

  @property({ type: Boolean, reflect: true }) public vertical = false;

  @property({ attribute: "icon-on" }) public iconOn = "mdi:lightbulb";

  @property({ attribute: "icon-off" }) public iconOff = "mdi:lightbulb-outline";

  private _toggle(): void {
    if (this.disabled) return;
    this.checked = !this.checked;
    this.dispatchEvent(new Event("change", { bubbles: true, composed: true }));
  }

  private _onKeyDown(ev: KeyboardEvent): void {
    if (ev.key === "Enter" || ev.key === " ") {
      ev.preventDefault();
      this._toggle();
    }
  }

  protected render() {
    return html`
      <div
        id="switch"
        class="switch"
        role="switch"
        tabindex=${this.disabled ? nothing : "0"}
        aria-checked=${this.checked ? "true" : "false"}
        aria-disabled=${this.disabled ? "true" : "false"}
        ?disabled=${this.disabled}
        @click=${this._toggle}
        @keydown=${this._onKeyDown}
      >
        <div class="background"></div>
        <div class="button">
          <ha-icon .icon=${this.checked ? this.iconOn : this.iconOff}></ha-icon>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      --control-switch-on-color: var(--feature-color, var(--tile-color, var(--primary-color)));
      --control-switch-off-color: var(--feature-color, var(--tile-color, var(--state-inactive-color)));
      --control-switch-background-opacity: 0.2;
      --control-switch-hover-background-opacity: 0.35;
      --control-switch-thickness: var(--feature-height, 40px);
      --control-switch-border-radius: var(
        --feature-border-radius,
        var(--ha-card-border-radius, 12px)
      );
      --control-switch-padding: 0;
      --mdc-icon-size: 20px;
      height: var(--control-switch-thickness);
      width: 100%;
      box-sizing: border-box;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }
    .switch {
      box-sizing: border-box;
      position: relative;
      height: 100%;
      width: 100%;
      border-radius: var(--control-switch-border-radius);
      outline: none;
      padding: var(--control-switch-padding);
      display: flex;
      cursor: pointer;
      overflow: hidden;
    }
    .switch:focus-visible {
      box-shadow: 0 0 0 2px var(--control-switch-off-color);
    }
    .switch[aria-checked="true"]:focus-visible {
      box-shadow: 0 0 0 2px var(--control-switch-on-color);
    }
    .switch[disabled] {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .background {
      position: absolute;
      inset: 0;
      border-radius: inherit;
      background-color: var(--control-switch-off-color);
      opacity: var(--control-switch-background-opacity);
      transition: background-color 180ms ease-in-out, opacity 180ms ease-in-out;
    }
    .switch:not([disabled]):hover .background {
      opacity: var(--control-switch-hover-background-opacity);
    }
    .switch[aria-checked="true"] .background {
      background-color: var(--control-switch-on-color);
    }
    .button {
      position: relative;
      z-index: 1;
      width: 50%;
      height: 100%;
      border-radius: calc(
        var(--control-switch-border-radius) - var(--control-switch-padding)
      );
      background-color: var(--control-switch-off-color);
      color: white;
      display: flex;
      align-items: center;
      justify-content: center;
      transition:
        transform 180ms ease-in-out,
        background-color 180ms ease-in-out;
    }
    .switch[aria-checked="true"] .button {
      transform: translateX(100%);
      background-color: var(--control-switch-on-color);
    }
    :host([vertical]) .switch[aria-checked="true"] .button {
      transform: translateY(100%);
    }
    :host([vertical]) {
      width: var(--control-switch-thickness);
      height: 100%;
    }
    :host([vertical]) .button {
      width: 100%;
      height: 50%;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-control-switch": HaControlSwitch;
  }
}
