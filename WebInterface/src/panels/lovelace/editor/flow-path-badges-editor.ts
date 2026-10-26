import { css, html, LitElement } from "lit";
import { customElement, property, query } from "lit/decorators.js";
import type { LovelacePathBadgeConfig } from "../../../types";
import {
  FlowBadgesListEditor,
  type BadgeListMode,
} from "./flow-badges-list-editor";
import "./flow-badges-list-editor";

@customElement("flow-path-badges-editor")
export class FlowPathBadgesEditor extends LitElement {
  @property({ attribute: false }) public badges: LovelacePathBadgeConfig[] = [];

  @query("flow-badges-list-editor") private _list?: FlowBadgesListEditor;

  get selectedIndex(): number {
    return this._list?.selectedIndex ?? -1;
  }

  private readonly _mode: BadgeListMode = "path";

  private _onChanged(ev: CustomEvent<{ badges: LovelacePathBadgeConfig[] }>): void {
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
        .mode=${this._mode}
        label="轨迹徽章"
        hint="状态徽章与控制徽章共用同一条轨迹；每个徽章可绑定各自的实体、显示方式和进度来源。"
        emptyText="还没有徽章。至少添加一个徽章后，轨迹上才会显示运动标记。"
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
    "flow-path-badges-editor": FlowPathBadgesEditor;
  }
}
