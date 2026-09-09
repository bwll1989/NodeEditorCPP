import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import {
  downloadCurrentFlow,
  fetchAppInfo,
  fetchCurrentFlowName,
  uploadFileBinary,
} from "../../data/setting-api";
import {
  isLayoutEditEnabled,
  setLayoutEditEnabled,
} from "../../data/layout-edit-mode";
import {
  checkSettingPassword,
  isSettingAuthedRecently,
  markSettingAuthed,
  verifySettingPassword,
} from "../../data/setting-auth";
import "../../components/ha-card";

const DEFAULT_MAX_COLUMNS = 2;
interface ListedFile {
  file: File;
  status?: "ready" | "success" | "fail";
}

@customElement("flow-settings-panel")
export class FlowSettingsPanel extends LitElement {
  @property({ attribute: false }) public flow?: import("../../types").Flow;

  @state() private _loading = true;
  @state() private _authed = false;
  @state() private _passwordRequired = true;
  @state() private _authPassword = "";
  @state() private _authError = "";
  @state() private _authLoading = false;
  @state() private _layoutEditEnabled = false;

  @state() private _appName = "Flow";
  @state() private _appVersion = "…";
  @state() private _currentFlowName = "检测中...";

  @state() private _flowFile: ListedFile | null = null;
  @state() private _flowStatus = "";
  @state() private _downloadStatus = "";
  @state() private _flowUploading = false;
  @state() private _downloading = false;
  @state() private _flowProgress = { show: false, percent: 0 };
  @state() private _columnCount = 1;

  private _resizeObserver?: ResizeObserver;

  connectedCallback(): void {
    super.connectedCallback();
    this._resizeObserver = new ResizeObserver((entries) => {
      const width = entries[0]?.contentRect.width ?? 0;
      if (!width) return;
      const minCol = 320;
      const gap = 32;
      const cols = Math.max(
        1,
        Math.min(DEFAULT_MAX_COLUMNS, Math.floor((width + gap) / (minCol + gap))),
      );
      if (cols !== this._columnCount) {
        this._columnCount = cols;
      }
    });
    this._resizeObserver.observe(this);
    this._layoutEditEnabled = isLayoutEditEnabled();
    void this._loadMeta();
  }
  disconnectedCallback(): void {
    this._resizeObserver?.disconnect();
    super.disconnectedCallback();
  }

  private _gridStyle() {
    return `--column-count: ${this._columnCount}`;
  }

  private async _loadMeta(): Promise<void> {
    this._loading = true;
    try {
      const info = await fetchAppInfo();
      this._appName = info.name;
      this._appVersion = info.version;
      this._currentFlowName = await fetchCurrentFlowName();
      await this._refreshAuthState();
    } finally {
      this._loading = false;
    }
  }

  private async _refreshAuthState(): Promise<void> {
    if (isSettingAuthedRecently()) {
      this._authed = true;
      return;
    }

    const emptyCheck = await checkSettingPassword("");
    if (emptyCheck === "ok" || emptyCheck === "offline") {
      markSettingAuthed();
      this._authed = true;
      this._passwordRequired = false;
      return;
    }

    this._authed = false;
    this._passwordRequired = true;
  }

  private async _submitAuth(): Promise<void> {
    this._authLoading = true;
    this._authError = "";
    try {
      const ok = await verifySettingPassword(this._authPassword);
      if (ok) {
        markSettingAuthed();
        this._authed = true;
        this._authPassword = "";
        return;
      }
      this._authError = "密码错误，请重试";
    } finally {
      this._authLoading = false;
    }
  }

  private _onAuthKeydown(ev: KeyboardEvent): void {
    if (ev.key === "Enter") void this._submitAuth();
  }

  private _toggleLayoutEdit(ev: Event): void {
    if (!this._authed) return;
    const enabled = (ev.target as HTMLInputElement).checked;
    this._layoutEditEnabled = enabled;
    setLayoutEditEnabled(enabled);
  }
  private _onFlowInput(ev: Event): void {
    const input = ev.target as HTMLInputElement;
    const file = input.files?.[0];
    this._flowFile = file ? { file, status: "ready" } : null;
    input.value = "";
  }

  private async _uploadFlow(): Promise<void> {
    const item = this._flowFile;
    const file = item?.file;
    if (!file) {
      this._flowStatus = "请选择.flow文件";
      return;
    }
    if (!file.name.toLowerCase().endsWith(".flow")) {
      this._flowStatus = "文件扩展名必须为 .flow";
      return;
    }

    this._flowUploading = true;
    this._flowProgress = { show: true, percent: 0 };
    this._flowStatus = "开始上传...";

    try {
      const url = `/api/upload/flow?filename=${encodeURIComponent(file.name)}`;
      const result = await uploadFileBinary(url, file, (pct) => {
        this._flowProgress = { show: true, percent: pct };
      });
      this._flowProgress = { show: true, percent: 100 };
      this._flowStatus = `上传成功：${result.path}`;
      this._flowFile = { file, status: "success" };
    } catch (e) {
      this._flowStatus = `上传失败：${e instanceof Error ? e.message : e}`;
      this._flowFile = { file, status: "fail" };
    } finally {
      this._flowUploading = false;
    }
  }

  private async _downloadFlow(): Promise<void> {
    this._downloadStatus = "正在请求...";
    this._downloading = true;
    try {
      const { filename, blob } = await downloadCurrentFlow();
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      a.remove();
      URL.revokeObjectURL(url);
      this._downloadStatus = "下载完成";
    } catch (e) {
      this._downloadStatus = `下载失败：${e instanceof Error ? e.message : e}`;
    } finally {
      this._downloading = false;
    }
  }

  private _renderAuthCard() {
    return html`
      <div class="grid-slot">
        <ha-card>
          <div class="card-body">
            <h2 class="card-title">访问验证</h2>
            ${this._authed
              ? html`
                  <p class="help auth-ok">已通过验证，可使用下方设置项。</p>
                  <span class="tag success">已解锁</span>
                `
              : html`
                  <p class="help">请输入设置密码以解锁上传、下载与编辑模式等操作。</p>
                  <input
                    class="text-input"
                    type="password"
                    .value=${this._authPassword}
                    placeholder="请输入设置页面密码"
                    autocomplete="current-password"
                    ?disabled=${this._authLoading}
                    @input=${(ev: Event) => {
                      this._authPassword = (ev.target as HTMLInputElement).value;
                    }}
                    @keydown=${this._onAuthKeydown}
                  />
                  ${this._authError
                    ? html`<div class="alert error">${this._authError}</div>`
                    : nothing}
                  <div class="actions">
                    <button
                      type="button"
                      class="btn primary"
                      ?disabled=${this._authLoading}
                      @click=${this._submitAuth}
                    >
                      ${this._authLoading ? "验证中..." : "验证并解锁"}
                    </button>
                  </div>
                `}
          </div>
        </ha-card>
      </div>
    `;
  }

  private _renderLockedSlot(content: unknown) {
    return html`
      <div class="grid-slot ${this._authed ? "" : "locked"}">
        <div class="card-lock-wrap" ?inert=${!this._authed}>${content}</div>
      </div>
    `;
  }

  private _renderMain() {
    return html`
      <div class="wrapper" style=${this._gridStyle()}>
        <div class="grid">
          ${this._passwordRequired ? this._renderAuthCard() : nothing}

          ${this._renderLockedSlot(html`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">编辑模式</h2>
                <p class="help">开启后，仪表盘右上角会出现编辑按钮，可修改布局与卡片。</p>
                <label class="switch-row">
                  <span>允许编辑仪表盘</span>
                  <input
                    type="checkbox"
                    .checked=${this._layoutEditEnabled}
                    ?disabled=${!this._authed}
                    @change=${this._toggleLayoutEdit}
                  />
                </label>
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(html`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">上传项目文件（.flow）</h2>
                <div class="flow-upload-row">
                  <label class="btn primary file-label">
                    选择文件
                    <input
                      type="file"
                      accept=".flow"
                      ?disabled=${this._flowUploading}
                      @change=${this._onFlowInput}
                    />
                  </label>
                  <button
                    type="button"
                    class="btn success"
                    ?disabled=${this._flowUploading}
                    @click=${this._uploadFlow}
                  >
                    ${this._flowUploading ? "上传中..." : "上传到服务器"}
                  </button>
                </div>
                <p class="upload-tip">仅允许单个 .flow 项目文件</p>
                ${this._flowFile
                  ? html`<div class="file-chip ${this._flowFile.status ?? ""}">${this._flowFile.file.name}</div>`
                  : nothing}
                ${this._flowStatus
                  ? html`<div class="status block">${this._flowStatus}</div>`
                  : nothing}
                ${this._flowProgress.show
                  ? html`
                      <div class="progress-wrap">
                        <div class="progress-bar">
                          <div
                            class="progress-fill success"
                            style=${`width:${this._flowProgress.percent}%`}
                          ></div>
                        </div>
                      </div>
                    `
                  : nothing}
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(html`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">下载当前项目</h2>
                <p class="help">下载当前正在运行的项目文件</p>
                <p class="help">
                  当前文件：<span class="tag">${this._currentFlowName}</span>
                </p>
                <div class="actions">
                  <button
                    type="button"
                    class="btn success"
                    ?disabled=${this._downloading}
                    @click=${this._downloadFlow}
                  >
                    ${this._downloading ? "下载中..." : "下载文件"}
                  </button>
                  <span class="status">${this._downloadStatus}</span>
                </div>
              </div>
            </ha-card>
          `)}

          ${this._renderLockedSlot(html`
            <ha-card>
              <div class="card-body">
                <h2 class="card-title">关于软件</h2>
                <div class="about-row">
                  <div class="about-meta">
                    <strong>${this._appName}</strong>
                    <span>版本 ${this._appVersion}</span>
                  </div>
                  <span class="tag success">v${this._appVersion}</span>
                </div>
              </div>
            </ha-card>
          `)}
        </div>
      </div>
    `;
  }

  protected render() {
    return html`
      <div class="page">
        ${this._loading
          ? html`<div class="wrapper" style=${this._gridStyle()}>
              <div class="grid">
                <div class="grid-slot">
                  <ha-card>
                    <div class="card-body">
                      <div class="checking">加载中...</div>
                    </div>
                  </ha-card>
                </div>
              </div>
            </div>`
          : this._renderMain()}
      </div>
    `;
  }

  static styles = css`
    :host {
      --row-gap: var(--ha-view-sections-row-gap, 16px);
      --column-gap: var(--ha-view-sections-column-gap, 24px);
      --column-max-width: var(--ha-view-sections-column-max-width, 500px);
      --column-min-width: var(--ha-view-sections-column-min-width, 320px);
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
      padding: 0 var(--column-gap);
      box-sizing: content-box;
      margin: var(--ha-view-sections-extra-top-margin, 16px) auto 0;
      max-width: calc(
        var(--column-count) * var(--column-max-width) +
          (var(--column-count) - 1) * var(--column-gap)
      );
    }
    /* 多列瀑布流：按高度自动补齐，避免网格行对齐留下空白 */
    .grid {
      column-count: var(--column-count);
      column-gap: var(--column-gap);
      column-fill: balance;
      padding: var(--row-gap) 0 calc(var(--row-gap) * 2);
    }
    .grid-slot {
      display: inline-block;
      width: 100%;
      min-width: 0;
      margin: 0 0 var(--row-gap);
      break-inside: avoid;
      page-break-inside: avoid;
      -webkit-column-break-inside: avoid;
    }
    .grid-slot.locked .card-lock-wrap {
      opacity: 0.42;
      pointer-events: none;
      user-select: none;
      filter: grayscale(0.35);
    }
    .auth-ok {
      margin-bottom: 6px;
    }
    .text-input {
      width: 100%;
      box-sizing: border-box;
      height: 36px;
      border: 1px solid #d1d5db;
      border-radius: 8px;
      padding: 0 12px;
      font: inherit;
    }
    .text-input:focus {
      outline: none;
      border-color: #03a9f4;
      box-shadow: 0 0 0 2px rgba(3, 169, 244, 0.15);
    }
    .alert {
      margin-top: 8px;
      padding: 8px 10px;
      border-radius: 8px;
      font-size: 13px;
    }
    .alert.error {
      background: #fef2f2;
      color: #b91c1c;
      border: 1px solid #fecaca;
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 4px 0 0;
      font-size: 14px;
      cursor: pointer;
    }
    .switch-row input {
      width: 18px;
      height: 18px;
      cursor: pointer;
      accent-color: #03a9f4;
    }
    .card-body {
      padding: 14px 16px;
    }
    .card-title {
      margin: 0 0 8px;
      font-size: 15px;
      font-weight: 600;
    }
    .checking {
      text-align: center;
      padding: 12px 0;
      color: #64748b;
    }
    .help {
      margin: 0 0 10px;
      color: var(--secondary-text-color);
      font-size: 13px;
      line-height: 1.45;
    }
    .help.tight {
      margin-bottom: 8px;
      font-size: 12px;
    }
    .actions {
      display: flex;
      align-items: center;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 10px;
    }
    .status {
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
      word-break: break-all;
    }
    .status.block {
      margin-top: 8px;
      display: block;
    }
    .btn {
      border: 1px solid #d1d5db;
      background: #fff;
      color: #1f2937;
      border-radius: 8px;
      height: 36px;
      padding: 0 14px;
      cursor: pointer;
      font: inherit;
      font-size: 14px;
    }
    .btn:disabled {
      opacity: 0.6;
      cursor: default;
    }
    .btn.primary {
      background: #03a9f4;
      border-color: #03a9f4;
      color: #fff;
    }
    .btn.success {
      background: #22c55e;
      border-color: #22c55e;
      color: #fff;
    }
    .dropzone {
      position: relative;
      border: 1px dashed #c0c4cc;
      border-radius: 8px;
      background: #fafafa;
      padding: 16px 12px;
      text-align: center;
      color: #606266;
    }
    .dropzone.dragover {
      border-color: #03a9f4;
      background: rgba(3, 169, 244, 0.04);
    }
    .dropzone em {
      color: #03a9f4;
      font-style: normal;
    }
    .file-input {
      position: absolute;
      inset: 0;
      opacity: 0;
      cursor: pointer;
    }
    .upload-icon {
      font-size: 36px;
      color: #c0c4cc;
      margin-bottom: 4px;
      line-height: 1;
    }
    .upload-icon svg {
      width: 36px;
      height: 36px;
      display: block;
      margin: 0 auto;
    }
    .upload-tip {
      margin: 8px 0 0;
      font-size: 12px;
      color: #909399;
    }
    .file-list {
      list-style: none;
      margin: 12px 0 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .file-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      padding: 8px 10px;
      border-radius: 8px;
      background: #f8fafc;
      font-size: 13px;
    }
    .file-item.success {
      color: #15803d;
    }
    .file-item.fail {
      color: #b91c1c;
    }
    .link-btn {
      border: none;
      background: transparent;
      color: #03a9f4;
      cursor: pointer;
      font-size: 12px;
      padding: 0;
    }
    .flow-upload-row {
      display: flex;
      align-items: center;
      gap: 12px;
      flex-wrap: wrap;
    }
    .file-label {
      position: relative;
      overflow: hidden;
      display: inline-flex;
      align-items: center;
      justify-content: center;
    }
    .file-label input {
      position: absolute;
      inset: 0;
      opacity: 0;
      cursor: pointer;
    }
    .file-chip {
      display: inline-block;
      margin-top: 8px;
      padding: 6px 10px;
      border-radius: 8px;
      background: #f1f5f9;
      font-size: 13px;
    }
    .file-chip.success {
      background: #dcfce7;
      color: #15803d;
    }
    .file-chip.fail {
      background: #fee2e2;
      color: #b91c1c;
    }
    .tag {
      display: inline-block;
      padding: 2px 8px;
      border-radius: 6px;
      background: #f1f5f9;
      color: #475569;
      font-size: 12px;
    }
    .tag.success {
      background: #dcfce7;
      color: #15803d;
    }
    .about-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      flex-wrap: wrap;
    }
    .about-meta {
      font-size: 14px;
    }
    .about-meta span {
      margin-left: 8px;
      color: #64748b;
    }
    .progress-wrap {
      margin-top: 12px;
    }
    .progress-bar {
      height: 8px;
      background: #e5e7eb;
      border-radius: 999px;
      overflow: hidden;
    }
    .progress-fill {
      height: 100%;
      background: #03a9f4;
      transition: width 0.2s ease;
    }
    .progress-fill.success {
      background: #22c55e;
    }
    .progress-label {
      display: block;
      margin-top: 6px;
      font-size: 12px;
      color: var(--secondary-text-color);
    }

    @media (max-width: 600px) {
      .grid {
        grid-template-columns: 1fr;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-settings-panel": FlowSettingsPanel;
  }
}
