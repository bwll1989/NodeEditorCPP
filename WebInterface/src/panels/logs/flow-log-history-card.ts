import { css, html, LitElement, nothing } from "lit";
import { customElement, state } from "lit/decorators.js";
import type { FlowLogFile } from "../../data/logs-api";
import {
  downloadLogFile,
  fetchLogFiles,
  formatLogFileSize,
  formatLogFileTime,
} from "../../data/logs-api";
import "../../components/ha-card";

@customElement("flow-log-history-card")
export class FlowLogHistoryCard extends LitElement {
  @state() private _files: FlowLogFile[] = [];
  @state() private _loading = true;
  @state() private _error = "";
  @state() private _downloading = "";

  connectedCallback(): void {
    super.connectedCallback();
    void this._load();
  }

  private async _load(): Promise<void> {
    this._loading = true;
    this._error = "";
    const result = await fetchLogFiles();
    this._loading = false;
    if (!result.ok) {
      this._error = result.error || "加载失败";
      this._files = [];
      return;
    }
    this._files = result.items;
  }

  private async _download(file: FlowLogFile): Promise<void> {
    this._downloading = file.name;
    this._error = "";
    const ok = await downloadLogFile(file.name);
    this._downloading = "";
    if (!ok) {
      this._error = `下载失败：${file.name}`;
    }
  }

  protected render() {
    return html`
      <ha-card>
        <div class="card-body">
          <div class="toolbar">
            <div>
              <h2 class="card-title">历史记录</h2>
              <p class="help">下载磁盘上按天保存的日志文件（Documents/Flow/Logs）。</p>
            </div>
            <button type="button" class="ghost" ?disabled=${this._loading} @click=${() => void this._load()}>
              刷新列表
            </button>
          </div>

          ${this._error ? html`<p class="error">${this._error}</p>` : nothing}

          ${this._loading
            ? html`<p class="empty">加载中…</p>`
            : this._files.length === 0
              ? html`<p class="empty">暂无历史日志文件</p>`
              : html`
                  <ul class="file-list">
                    ${this._files.map(
                      (file) => html`
                        <li class="file-row">
                          <div class="file-meta">
                            <span class="file-name">${file.name}</span>
                            <span class="file-sub">
                              ${formatLogFileTime(file.modified)} · ${formatLogFileSize(file.size)}
                            </span>
                          </div>
                          <button
                            type="button"
                            class="download"
                            ?disabled=${this._downloading === file.name}
                            @click=${() => void this._download(file)}
                          >
                            ${this._downloading === file.name ? "下载中…" : "下载"}
                          </button>
                        </li>
                      `,
                    )}
                  </ul>
                `}
        </div>
      </ha-card>
    `;
  }

  static styles = css`
    :host {
      display: flex;
      flex: 1;
      min-height: 0;
      min-width: 0;
      max-height: 100%;
      overflow: hidden;
    }
    @media (max-width: 870px) {
      :host {
        flex: none;
        max-height: none;
        height: 100%;
        min-height: 240px;
      }
    }
    ha-card {
      flex: 1;
      min-height: 0;
      max-height: 100%;
      overflow: hidden;
      display: flex;
      flex-direction: column;
    }
    .card-body {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      padding: 16px 18px;
      overflow: hidden;
    }
    .toolbar {
      display: flex;
      flex-wrap: wrap;
      align-items: flex-start;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 8px;
      flex-shrink: 0;
    }
    .card-title {
      margin: 0 0 4px;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 0;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 13px;
      line-height: 1.45;
    }
    button.ghost,
    button.download {
      padding: 6px 12px;
      border-radius: 8px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      background: var(--card-background-color, #fff);
      font-size: 13px;
      cursor: pointer;
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
    }
    button.download {
      min-width: 72px;
      border-color: rgba(3, 169, 244, 0.35);
      color: var(--primary-color, #0288d1);
      background: rgba(3, 169, 244, 0.06);
    }
    button:disabled {
      opacity: 0.6;
      cursor: default;
    }
    .error {
      color: var(--error-color, #c62828);
      font-size: 13px;
      margin: 0 0 8px;
    }
    .empty {
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
      margin: 12px 0 0;
      font-size: 13px;
    }
    .file-list {
      list-style: none;
      margin: 0;
      padding: 0;
      flex: 1;
      min-height: 0;
      overflow: auto;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .file-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 10px 12px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.08));
      border-radius: 10px;
      background: var(--secondary-background-color, #f7f9fb);
    }
    .file-meta {
      min-width: 0;
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    .file-name {
      font-family: ui-monospace, "Cascadia Code", "Consolas", monospace;
      font-size: 13px;
      word-break: break-all;
    }
    .file-sub {
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.54));
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-log-history-card": FlowLogHistoryCard;
  }
}
