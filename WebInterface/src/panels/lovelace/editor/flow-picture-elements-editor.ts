import { css, html, LitElement } from "lit";
import { customElement, property, query } from "lit/decorators.js";
import type { LovelacePictureElementConfig } from "../../../types";
import {
  FlowBadgesListEditor,
  type BadgeListMode,
} from "./flow-badges-list-editor";
import "./flow-badges-list-editor";

@customElement("flow-picture-elements-editor")
export class FlowPictureElementsEditor extends LitElement {
  @property({ attribute: false }) public elements: LovelacePictureElementConfig[] = [];

  @query("flow-badges-list-editor") private _list?: FlowBadgesListEditor;

  private readonly _mode: BadgeListMode = "picture";

  get selectedIndex(): number {
    return this._list?.selectedIndex ?? -1;
  }

  applyPosition(left: number, top: number): void {
    this._list?.applyPicturePosition(left, top);
  }

  selectIndex(index: number): void {
    this._list?.selectIndex(index);
  }

  private _onChanged(ev: CustomEvent<{ badges: LovelacePictureElementConfig[] }>): void {
    ev.stopPropagation();
    this.dispatchEvent(
      new CustomEvent("elements-changed", {
        bubbles: true,
        composed: true,
        detail: { elements: ev.detail.badges },
      }),
    );
  }

  protected render() {
    return html`
      <flow-badges-list-editor
        .mode=${this._mode}
        label="元素"
        hint="点上方徽章切换编辑；可在右侧预览图上点击或拖动定位。"
        emptyText="还没有元素。添加后会叠在底图上。"
        .badges=${this.elements}
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
    "flow-picture-elements-editor": FlowPictureElementsEditor;
  }
}
