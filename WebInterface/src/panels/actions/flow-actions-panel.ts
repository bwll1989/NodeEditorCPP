import { css, html, LitElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import type { Flow } from "../../types";
import "../../components/ha-card";
import "../lovelace/editor/flow-action-library";
import "../lovelace/editor/flow-manual-action-form";

@customElement("flow-actions-panel")
export class FlowActionsPanel extends LitElement {
  @property({ attribute: false }) public flow?: Flow;

  protected render() {
    return html`
      <div class="page">
        <div class="wrapper">
          <ha-card>
            <div class="card-body">
              <h2 class="card-title">动作库</h2>
              <p class="help">管理已发布到网页的控制地址，可删除错误或不需要的动作；点击「测试」可向地址发送一次测试值。</p>
              <flow-action-library .flow=${this.flow}></flow-action-library>
            </div>
          </ha-card>

          <ha-card>
            <div class="card-body">
              <h2 class="card-title">手动添加</h2>
              <flow-manual-action-form></flow-manual-action-form>
            </div>
          </ha-card>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      flex: 1;
      min-height: 0;
      overflow: auto;
    }
    .page {
      min-height: 100%;
      background: var(--primary-background-color, #e8edf1);
      color: var(--primary-text-color);
      font-family: var(--ha-font-family, system-ui, sans-serif);
    }
    .wrapper {
      max-width: 960px;
      margin: 16px auto 0;
      padding: 0 24px 32px;
      box-sizing: border-box;
      display: flex;
      flex-direction: row;
      align-items: stretch;
      gap: 16px;
    }
    .wrapper > ha-card {
      flex: 1 1 0;
      min-width: 0;
    }
    @media (max-width: 720px) {
      .wrapper {
        flex-direction: column;
      }
    }
    .card-body {
      padding: 16px 18px;
    }
    .card-title {
      margin: 0 0 8px;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 0 0 12px;
      color: var(--secondary-text-color);
      font-size: 13px;
      line-height: 1.45;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-actions-panel": FlowActionsPanel;
  }
}
