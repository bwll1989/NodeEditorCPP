import { css, LitElement, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import {
  DEFAULT_SECTION_BACKGROUND_OPACITY,
  resolveSectionBackground,
} from "../../../data/section-config";
import { computeCssColor } from "../../../common/color/theme-colors";
import type { LovelaceSectionBackgroundConfig } from "../../../types";

@customElement("flow-section-background")
export class FlowSectionBackground extends LitElement {
  @property({ attribute: false })
  public background?: boolean | LovelaceSectionBackgroundConfig;

  protected updated(): void {
    const resolved = resolveSectionBackground(this.background);
    if (!resolved) {
      this.style.removeProperty("--section-background-color");
      this.style.removeProperty("--section-background-opacity");
      return;
    }
    const color = resolved.color && resolved.color !== "default" ? resolved.color : null;
    this.style.setProperty(
      "--section-background-color",
      color
        ? computeCssColor(color)
        : "var(--ha-section-background-color, var(--secondary-background-color))",
    );
    const opacity =
      resolved.opacity !== undefined ? resolved.opacity : DEFAULT_SECTION_BACKGROUND_OPACITY;
    this.style.setProperty("--section-background-opacity", `${opacity}%`);
  }

  protected render() {
    return nothing;
  }

  static styles = css`
    :host {
      position: absolute;
      inset: 0;
      border-radius: inherit;
      background-color: var(--section-background-color);
      opacity: var(--section-background-opacity, 50%);
      z-index: 0;
      pointer-events: none;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-section-background": FlowSectionBackground;
  }
}
