import { css, html, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import "./ha-icon";

/** Port of HA ha-tile-icon */
@customElement("ha-tile-icon")
export class HaTileIcon extends LitElement {
  @property() public icon = "";

  @property({ type: Boolean, reflect: true }) public interactive = false;

  @property({ type: Boolean }) public active = false;

  @property({ type: Boolean }) public large = false;

  protected render() {
    return html`
      <div
        class="container background ${this.interactive ? "button" : ""}"
        role=${this.interactive ? "button" : nothing}
        tabindex=${this.interactive ? "0" : nothing}
      >
        <ha-icon .icon=${this.icon}></ha-icon>
        <slot></slot>
      </div>
    `;
  }

  static styles = css`
    :host {
      --tile-icon-color: var(--state-inactive-color);
      --tile-icon-opacity: 0.2;
      --tile-icon-hover-opacity: 0.35;
      --tile-icon-border-radius: var(
        --ha-tile-icon-border-radius,
        var(--ha-border-radius-pill, 999px)
      );
      --tile-icon-size: 36px;
      --mdc-icon-size: 24px;
      position: relative;
      display: block;
      user-select: none;
      pointer-events: none;
      flex-shrink: 0;
      transition: transform 180ms ease-in-out;
    }
    :host([interactive]) {
      -webkit-tap-highlight-color: transparent;
      pointer-events: auto;
      cursor: pointer;
    }
    :host([interactive]:active) .container {
      transform: scale(1.08);
    }
    :host([interactive]:hover) {
      --tile-icon-opacity: var(--tile-icon-hover-opacity);
    }
    :host([large]) {
      --tile-icon-size: 56px;
      --mdc-icon-size: 32px;
    }
    .container {
      position: relative;
      display: flex;
      align-items: center;
      justify-content: center;
      width: var(--tile-icon-size);
      height: var(--tile-icon-size);
      border-radius: var(--tile-icon-border-radius);
      overflow: hidden;
      box-sizing: border-box;
      transition: box-shadow 180ms ease-in-out, transform 180ms ease-in-out;
    }
    .container.button {
      pointer-events: auto;
      cursor: pointer;
    }
    .container.button:focus-visible {
      outline: none;
      box-shadow: 0 0 0 2px var(--tile-icon-color);
    }
    .container.background::before {
      content: "";
      position: absolute;
      inset: 0;
      background-color: var(--tile-icon-color);
      opacity: var(--tile-icon-opacity);
      transition:
        background-color 180ms ease-in-out,
        opacity 180ms ease-in-out;
    }
    ha-icon {
      position: relative;
      z-index: 1;
      display: flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size);
      height: var(--mdc-icon-size);
      color: var(--tile-icon-foreground, var(--tile-icon-color));
      pointer-events: none;
      transition: color 180ms ease-in-out;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-tile-icon": HaTileIcon;
  }
}
