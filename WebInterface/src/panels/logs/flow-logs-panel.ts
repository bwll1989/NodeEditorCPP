import { css, html, LitElement, nothing } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import type { PropertyValues } from "lit";
import type { FlowLogEntry, LogLevelFilter } from "../../data/logs-api";
import {
  fetchLogTail,
  levelClass,
} from "../../data/logs-api";
import "../../components/ha-card";
import "./flow-log-history-card";

const LEVELS: LogLevelFilter[] = ["All", "Debug", "Info", "Warn", "Critical", "Fatal"];

@customElement("flow-logs-panel")
export class FlowLogsPanel extends LitElement {
  @property({ type: Boolean }) public wsConnected = false;

  @state() private _entries: FlowLogEntry[] = [];
  @state() private _levelFilter: LogLevelFilter = "All";
  @state() private _autoScroll = true;
  @state() private _loading = true;
  @state() private _error = "";
  @state() private _lastSeq = 0;

  @query(".log-view") private _logView?: HTMLElement;

  private _ignoreScrollEvent = false;

  private readonly _onLogEntry = (ev: Event): void => {
    const detail = (ev as CustomEvent<FlowLogEntry>).detail;
    if (!detail || typeof detail.seq !== "number") return;
    if (detail.seq <= this._lastSeq) return;
    if (this._levelFilter !== "All" && detail.level !== this._levelFilter) return;

    this._lastSeq = detail.seq;
    this._entries = [...this._entries, detail].slice(-1000);
  };

  connectedCallback(): void {
    super.connectedCallback();
    document.addEventListener("flow-log-entry", this._onLogEntry as EventListener);
    void this._loadInitial();
  }

  disconnectedCallback(): void {
    document.removeEventListener("flow-log-entry", this._onLogEntry as EventListener);
    super.disconnectedCallback();
  }

  protected updated(changed: PropertyValues): void {
    if (changed.has("_entries") || (changed.has("_loading") && !this._loading)) {
      this._scrollToBottom();
    }
    if (changed.has("wsConnected") && this.wsConnected) {
      void this._syncSinceLast();
    }
  }

  private async _loadInitial(): Promise<void> {
    this._loading = true;
    this._error = "";

    const result = await fetchLogTail({ limit: 300, level: this._levelFilter });
    this._loading = false;
    if (!result.ok) {
      this._error = result.error || "加载失败";
      return;
    }
    this._applyEntries(result.items);
  }

  private async _syncSinceLast(): Promise<void> {
    const result = await fetchLogTail({
      limit: 500,
      level: this._levelFilter,
      since: this._lastSeq,
    });
    if (!result.ok || result.items.length === 0) return;
    this._applyEntries([...this._entries, ...result.items].slice(-1000));
  }

  private _applyEntries(items: FlowLogEntry[]): void {
    const sorted = [...items].sort((a, b) => a.seq - b.seq);
    this._entries = sorted;
    if (sorted.length > 0) {
      this._lastSeq = sorted[sorted.length - 1]!.seq;
    }
  }

  private _scrollToBottom(): void {
    if (!this._autoScroll || this._loading || this._entries.length === 0) {
      return;
    }

    void this.updateComplete.then(() => {
      requestAnimationFrame(() => {
        const el = this._logView;
        if (!el) return;

        this._ignoreScrollEvent = true;
        const lastRow = el.querySelector(".log-row:last-child");
        if (lastRow) {
          lastRow.scrollIntoView({ block: "end" });
        } else {
          el.scrollTop = el.scrollHeight;
        }
        requestAnimationFrame(() => {
          this._ignoreScrollEvent = false;
        });
      });
    });
  }

  private _onScroll = (): void => {
    if (this._ignoreScrollEvent) return;
    const el = this._logView;
    if (!el) return;
    const atBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 48;
    this._autoScroll = atBottom;
  };

  private _resumeScroll(): void {
    this._autoScroll = true;
    this._scrollToBottom();
  }

  private async _onFilterChange(ev: Event): Promise<void> {
    this._levelFilter = (ev.target as HTMLSelectElement).value as LogLevelFilter;
    this._entries = [];
    this._lastSeq = 0;
    await this._loadInitial();
  }

  private _clearDisplay(): void {
    this._entries = [];
  }

  protected render() {
    return html`
      <div class="page">
        <div class="wrapper">
          <ha-card>
            <div class="card-body">
              <div class="toolbar">
                <div class="toolbar-left">
                  <h2 class="card-title">运行日志</h2>
                  <span class="status ${this.wsConnected ? "live" : "offline"}">
                    ${this.wsConnected ? "实时" : "离线"}
                  </span>
                  ${!this._autoScroll
                    ? html`
                        <button type="button" class="resume" @click=${this._resumeScroll}>
                          回到底部
                        </button>
                      `
                    : nothing}
                </div>
                <div class="toolbar-right">
                  <label class="filter">
                    <span>级别</span>
                    <select .value=${this._levelFilter} @change=${this._onFilterChange}>
                      ${LEVELS.map(
                        (level) => html`<option value=${level}>${level}</option>`,
                      )}
                    </select>
                  </label>
                  <button type="button" class="ghost" @click=${this._clearDisplay}>清空显示</button>
                  <button type="button" class="ghost" @click=${() => void this._loadInitial()}>
                    刷新
                  </button>
                </div>
              </div>
              <p class="help">
                与桌面「终端显示」同步；WebSocket 连接时自动追加新日志。向上滚动可暂停自动滚底。
              </p>
              ${this._error ? html`<p class="error">${this._error}</p>` : nothing}
              <div class="log-shell">
                <div class="log-view" @scroll=${this._onScroll}>
                  ${this._loading
                    ? html`<p class="empty">加载中…</p>`
                    : this._entries.length === 0
                      ? html`<p class="empty">暂无日志</p>`
                      : this._entries.map(
                          (entry) => html`
                            <div class="log-row ${levelClass(entry.level)}">
                              <span class="ts">${entry.timestamp}</span>
                              <span class="lvl">${entry.level}</span>
                              <span class="msg">${entry.message}</span>
                            </div>
                          `,
                        )}
                </div>
              </div>
            </div>
          </ha-card>
          <flow-log-history-card></flow-log-history-card>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      flex: 1;
      min-height: 0;
      overflow: hidden;
      --logs-panel-max-height: min(680px, calc(100dvh - var(--header-height, 56px) - 88px));
    }
    .page {
      height: 100%;
      min-height: 0;
      display: flex;
      flex-direction: column;
      overflow-y: auto;
      overflow-x: hidden;
      background: var(--primary-background-color, #e8edf1);
      color: var(--primary-text-color);
      font-family: var(--ha-font-family, system-ui, sans-serif);
    }
    .wrapper {
      flex: 0 0 auto;
      width: 100%;
      max-width: 1400px;
      max-height: var(--logs-panel-max-height);
      height: var(--logs-panel-max-height);
      margin: 0 auto;
      padding: 16px 24px 24px;
      box-sizing: border-box;
      display: flex;
      flex-direction: row;
      align-items: stretch;
      gap: 16px;
      overflow: hidden;
    }
    .wrapper > ha-card {
      flex: 2 1 0;
      min-width: 0;
      min-height: 0;
      display: flex;
      flex-direction: column;
    }
    .wrapper > flow-log-history-card {
      flex: 1 1 0;
      min-width: 0;
      min-height: 0;
      display: flex;
      flex-direction: column;
    }
    @media (max-width: 870px) {
      :host {
        flex: none;
        height: auto;
        min-height: 0;
        overflow: visible;
        --logs-panel-max-height: none;
      }
      .page {
        height: auto;
        min-height: 0;
        overflow: visible;
      }
      .wrapper {
        flex-direction: column;
        height: auto;
        max-height: none;
        overflow: visible;
        padding: 16px 16px 24px;
      }
      .wrapper > ha-card {
        flex: none;
        width: 100%;
        height: min(420px, calc(100dvh - var(--header-height, 56px) - 120px));
        min-height: 280px;
        max-height: none;
      }
      .wrapper > flow-log-history-card {
        flex: none;
        width: 100%;
        height: min(320px, calc(100dvh - var(--header-height, 56px) - 120px));
        min-height: 240px;
        max-height: none;
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
    .card-title {
      margin: 0;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 8px 0 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 13px;
      line-height: 1.45;
      flex-shrink: 0;
    }
    .toolbar {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 4px;
      flex-shrink: 0;
    }
    .toolbar-left,
    .toolbar-right {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 10px;
    }
    .status {
      font-size: 12px;
      padding: 2px 10px;
      border-radius: 999px;
      font-weight: 500;
      line-height: 1.6;
    }
    .status.live {
      background: rgba(46, 125, 50, 0.12);
      color: #2e7d32;
    }
    .status.offline {
      background: rgba(0, 0, 0, 0.06);
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.54));
    }
    .resume {
      padding: 2px 10px;
      border: 1px solid var(--primary-color, #03a9f4);
      border-radius: 999px;
      background: rgba(3, 169, 244, 0.08);
      color: var(--primary-color, #0288d1);
      font-size: 12px;
      cursor: pointer;
    }
    .filter {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 13px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
    }
    select,
    button.ghost {
      padding: 6px 12px;
      border-radius: 8px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      background: var(--card-background-color, #fff);
      font-size: 13px;
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
      cursor: pointer;
    }
    .error {
      color: var(--error-color, #c62828);
      font-size: 13px;
      margin: 0 0 8px;
    }
    .log-shell {
      flex: 1;
      min-height: 0;
      display: flex;
      flex-direction: column;
      margin-top: 4px;
      padding: 1px;
      border-radius: 12px;
      overflow: hidden;
      background: linear-gradient(
        180deg,
        rgba(0, 0, 0, 0.06) 0%,
        rgba(0, 0, 0, 0.03) 100%
      );
    }
    .log-view {
      flex: 1;
      min-height: 0;
      overflow: auto;
      padding: 12px 14px;
      border-radius: 11px;
      background: var(--log-surface, #f7f9fb);
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.08));
      box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.03);
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
      font-family: ui-monospace, "Cascadia Code", "Consolas", monospace;
      font-size: 12px;
      line-height: 1.55;
    }
    .log-row {
      display: grid;
      grid-template-columns: 92px 76px minmax(0, 1fr);
      gap: 12px;
      align-items: start;
      padding: 6px 8px;
      border-radius: 6px;
      transition: background 0.12s ease;
    }
    .log-row:hover {
      background: rgba(0, 0, 0, 0.03);
    }
    .log-row + .log-row {
      margin-top: 2px;
    }
    .ts {
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
      white-space: nowrap;
      font-variant-numeric: tabular-nums;
    }
    .lvl {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 52px;
      padding: 1px 8px;
      border-radius: 6px;
      font-size: 11px;
      font-weight: 600;
      line-height: 1.5;
      white-space: nowrap;
    }
    .msg {
      word-break: break-word;
      white-space: pre-wrap;
      color: rgba(0, 0, 0, 0.78);
    }
    .level-debug .lvl {
      background: rgba(3, 169, 244, 0.12);
      color: #0277bd;
    }
    .level-info .lvl {
      background: rgba(2, 136, 209, 0.12);
      color: #01579b;
    }
    .level-warn .lvl {
      background: rgba(255, 152, 0, 0.14);
      color: #e65100;
    }
    .level-critical .lvl {
      background: rgba(211, 47, 47, 0.12);
      color: #b71c1c;
    }
    .empty {
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
      margin: 0;
      padding: 24px 8px;
      text-align: center;
    }
    @media (max-width: 720px) {
      .log-row {
        grid-template-columns: 1fr;
        gap: 4px;
      }
      .ts,
      .lvl {
        display: inline-flex;
      }
      .ts::after {
        content: " ·";
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-logs-panel": FlowLogsPanel;
  }
}
