import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import type { LovelaceHeadingBadgeConfig } from "../../../types";
import "./flow-badges-list-editor";

@customElement("flow-heading-badges-editor")
export class FlowHeadingBadgesEditor extends LitElement {
  @property({ attribute: false }) public badges: LovelaceHeadingBadgeConfig[] = [];

  private _onChanged(ev: CustomEvent<{ badges: LovelaceHeadingBadgeConfig[] }>): void {
    ev.stopPropagation();
    this.dispatchEvent(
      new CustomEvent("badges-changed", {
        bubbles: true,
        composed: true,
        detail: { badges: ev.detail.badges },
      }),
    );
  }

  protected render() {
    return html`
      <flow-badges-list-editor
        mode="heading"
        label="徽章"
        emptyText="可添加状态徽章（只读）或控制徽章（点击操作）"
        .badges=${this.badges}
        @badges-changed=${this._onChanged}
      ></flow-badges-list-editor>
    `;
  }

  static styles = css`
    :host {
      display: block;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-heading-badges-editor": FlowHeadingBadgesEditor;
  }
}
