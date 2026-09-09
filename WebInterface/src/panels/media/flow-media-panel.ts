import { css, html, LitElement, nothing } from "lit";
import { customElement, state } from "lit/decorators.js";
import {
  downloadMediaFile,
  fetchMediaFiles,
  formatMediaFileSize,
  formatMediaFileTime,
  groupMediaFiles,
  type FlowMediaFile,
} from "../../data/media-api";
import { MEDIA_ACCEPT, uploadFileBinary } from "../../data/setting-api";
import "../../components/ha-card";
import "../../components/ha-expandable-panel";

interface ListedFile {
  file: File;
  status?: "ready" | "success" | "fail";
}

@customElement("flow-media-panel")
export class FlowMediaPanel extends LitElement {
  @state() private _files: FlowMediaFile[] = [];
  @state() private _loading = true;
  @state() private _error = "";
  @state() private _downloading = "";

  @state() private _mediaFiles: ListedFile[] = [];
  @state() private _mediaStatus = "";
  @state() private _mediaUploading = false;
  @state() private _mediaProgress = { show: false, percent: 0, label: "" };

  private _mediaDragOver = false;

  connectedCallback(): void {
    super.connectedCallback();
    void this._loadLibrary();
  }

  private async _loadLibrary(): Promise<void> {
    this._loading = true;
    this._error = "";
    const result = await fetchMediaFiles();
    this._loading = false;
    if (!result.ok) {
      this._error = result.error || "加载失败";
      this._files = [];
      return;
    }
    this._files = result.items;
  }

  private _setMediaFiles(files: FileList | File[]): void {
    const next = Array.from(files).map((file) => ({ file, status: "ready" as const }));
    this._mediaFiles = next;
  }

  private _onMediaInput(ev: Event): void {
    const input = ev.target as HTMLInputElement;
    if (input.files?.length) this._setMediaFiles(input.files);
    input.value = "";
  }

  private _onMediaDrop(ev: DragEvent): void {
    ev.preventDefault();
    this._mediaDragOver = false;
    if (ev.dataTransfer?.files?.length) this._setMediaFiles(ev.dataTransfer.files);
  }

  private _removeMedia(index: number): void {
    this._mediaFiles = this._mediaFiles.filter((_, i) => i !== index);
  }

  private async _uploadMedia(): Promise<void> {
    const list = this._mediaFiles.map((item) => item.file);
    if (!list.length) {
      this._mediaStatus = "请选择媒体文件";
      return;
    }

    this._mediaUploading = true;
    this._mediaProgress = { show: true, percent: 0, label: "" };
    const results: { name: string; ok: boolean; error?: string }[] = [];

    try {
      for (let i = 0; i < list.length; i++) {
        const file = list[i];
        const label = `${i + 1}/${list.length} ${file.name}`;
        this._mediaProgress = { ...this._mediaProgress, label };
        this._mediaStatus = `正在上传：${label}`;
        const url = `/api/upload/media?filename=${encodeURIComponent(file.name)}`;
        try {
          await uploadFileBinary(url, file, (pct) => {
            const base = (i / list.length) * 100;
            this._mediaProgress = {
              show: true,
              label,
              percent: Math.min(100, Math.round(base + pct / list.length)),
            };
          });
          results.push({ name: file.name, ok: true });
          this._mediaFiles = this._mediaFiles.map((item, idx) =>
            idx === i ? { ...item, status: "success" } : item,
          );
        } catch (e) {
          const error = e instanceof Error ? e.message : String(e);
          results.push({ name: file.name, ok: false, error });
          this._mediaFiles = this._mediaFiles.map((item, idx) =>
            idx === i ? { ...item, status: "fail" } : item,
          );
        }
      }

      this._mediaProgress = { ...this._mediaProgress, percent: 100 };
      const okCount = results.filter((r) => r.ok).length;
      const failCount = results.length - okCount;
      this._mediaStatus =
        `完成：成功 ${okCount}，失败 ${failCount}` +
        (failCount
          ? `；失败：${results
              .filter((r) => !r.ok)
              .map((r) => `${r.name}(${r.error})`)
              .join("；")}`
          : "");

      if (okCount > 0) {
        await this._loadLibrary();
      }
    } catch (e) {
      this._mediaStatus = `上传异常：${e}`;
    } finally {
      this._mediaUploading = false;
    }
  }

  private async _download(file: FlowMediaFile): Promise<void> {
    this._downloading = file.name;
    this._error = "";
    const ok = await downloadMediaFile(file.name);
    this._downloading = "";
    if (!ok) {
      this._error = `下载失败：${file.name}`;
    }
  }

  private _renderUploadIcon() {
    return html`
      <div class="upload-icon" aria-hidden="true">
        <svg viewBox="0 0 24 24" width="32" height="32" fill="currentColor">
          <path
            d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20M12,12L16,16H13.5V19H10.5V16H8L12,12Z"
          />
        </svg>
      </div>
    `;
  }

  protected render() {
    return html`
      <div class="page">
        <div class="layout">
          <ha-card class="library-card">
            <div class="card-body">
              <div class="toolbar">
                <div>
                  <h2 class="card-title">媒体库</h2>
                  <p class="help">服务器上已保存的媒体文件，可下载到本地。</p>
                </div>
                <button type="button" class="ghost" ?disabled=${this._loading} @click=${() => void this._loadLibrary()}>
                  刷新列表
                </button>
              </div>

              ${this._error ? html`<p class="error">${this._error}</p>` : nothing}

              ${this._loading
                ? html`<p class="empty">加载中…</p>`
                : this._files.length === 0
                  ? html`<p class="empty">暂无媒体文件</p>`
                  : html`
                      <div class="file-groups">
                        ${groupMediaFiles(this._files).map(
                          (group) => html`
                            <ha-expandable-panel
                              class="file-group"
                              .header=${`${group.label} (${group.items.length})`}
                              expanded
                            >
                              <ul class="file-list">
                                ${group.items.map(
                                  (file) => html`
                                    <li class="file-row">
                                      <div class="file-meta">
                                        <span class="file-name">${file.name}</span>
                                        <span class="file-sub">
                                          ${formatMediaFileTime(file.modified)} · ${formatMediaFileSize(file.size)}
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
                            </ha-expandable-panel>
                          `,
                        )}
                      </div>
                    `}
            </div>
          </ha-card>

          <ha-card class="upload-card">
            <div class="card-body">
              <h2 class="card-title">上传媒体</h2>
              <p class="help">将文件拖拽到下方区域，或点击选择。支持多文件上传。</p>
              <div
                class="dropzone ${this._mediaDragOver ? "dragover" : ""}"
                @dragover=${(ev: DragEvent) => {
                  ev.preventDefault();
                  this._mediaDragOver = true;
                }}
                @dragleave=${() => {
                  this._mediaDragOver = false;
                }}
                @drop=${this._onMediaDrop}
              >
                ${this._renderUploadIcon()}
                <p>将文件拖到此处，或 <em>点击选择</em></p>
                <input
                  class="file-input"
                  type="file"
                  multiple
                  accept=${MEDIA_ACCEPT}
                  ?disabled=${this._mediaUploading}
                  @change=${this._onMediaInput}
                />
              </div>
              <p class="upload-tip">支持常见音视频 / 图片，可一次选择多个文件</p>
              ${this._mediaFiles.length
                ? html`
                    <ul class="pick-list">
                      ${this._mediaFiles.map(
                        (item, index) => html`
                          <li class="pick-item ${item.status ?? ""}">
                            <span>${item.file.name}</span>
                            <button
                              type="button"
                              class="link-btn"
                              ?disabled=${this._mediaUploading}
                              @click=${() => this._removeMedia(index)}
                            >
                              移除
                            </button>
                          </li>
                        `,
                      )}
                    </ul>
                  `
                : nothing}
              <div class="actions">
                <button
                  type="button"
                  class="btn primary"
                  ?disabled=${this._mediaUploading}
                  @click=${() => void this._uploadMedia()}
                >
                  ${this._mediaUploading ? "上传中…" : "上传到服务器"}
                </button>
                <span class="status">${this._mediaStatus}</span>
              </div>
              ${this._mediaProgress.show
                ? html`
                    <div class="progress-wrap">
                      <div class="progress-bar">
                        <div class="progress-fill" style=${`width:${this._mediaProgress.percent}%`}></div>
                      </div>
                      <span class="progress-label">
                        ${this._mediaProgress.label || `${this._mediaProgress.percent}%`}
                      </span>
                    </div>
                  `
                : nothing}
            </div>
          </ha-card>
        </div>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100%;
      min-height: 0;
    }
    .page {
      height: 100%;
      min-height: 0;
      padding: 16px 20px 24px;
      box-sizing: border-box;
    }
    .layout {
      display: grid;
      grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
      align-items: start;
      gap: 16px;
      height: 100%;
      max-height: min(680px, calc(100dvh - var(--header-height, 56px) - 88px));
      min-height: 0;
    }
    @media (max-width: 870px) {
      :host {
        height: auto;
        flex: none;
        overflow: visible;
      }
      .page {
        height: auto;
        min-height: 0;
        overflow: visible;
        padding: 16px 16px 24px;
      }
      .layout {
        grid-template-columns: 1fr;
        height: auto;
        max-height: none;
        align-items: stretch;
      }
      .library-card {
        height: min(420px, calc(100dvh - var(--header-height, 56px) - 120px));
        min-height: 280px;
        max-height: none;
      }
      .upload-card {
        height: auto;
      }
    }
    ha-card {
      min-height: 0;
      overflow: hidden;
      display: flex;
      flex-direction: column;
    }
    .library-card {
      height: 100%;
      max-height: inherit;
    }
    .upload-card {
      height: auto;
      align-self: start;
    }
    .upload-card .card-body {
      flex: none;
      overflow: visible;
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
      margin: 0 0 4px;
      font-size: 16px;
      font-weight: 600;
    }
    .help {
      margin: 0 0 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 13px;
      line-height: 1.45;
    }
    .btn.primary,
    button.ghost,
    button.download {
      padding: 8px 14px;
      border-radius: 8px;
      border: 1px solid var(--divider-color, rgba(0, 0, 0, 0.12));
      background: var(--card-background-color, #fff);
      font-size: 13px;
      cursor: pointer;
      color: var(--primary-text-color, rgba(0, 0, 0, 0.87));
    }
    .btn.primary {
      border-color: rgba(3, 169, 244, 0.35);
      color: var(--primary-color, #0288d1);
      background: rgba(3, 169, 244, 0.06);
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
    .dropzone {
      position: relative;
      border: 2px dashed var(--divider-color, rgba(0, 0, 0, 0.18));
      border-radius: 12px;
      padding: 16px 12px;
      text-align: center;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.55));
      background: var(--secondary-background-color, #f7f9fb);
      transition: border-color 0.15s, background 0.15s;
    }
    .dropzone.dragover {
      border-color: var(--primary-color, #0288d1);
      background: rgba(3, 169, 244, 0.06);
    }
    .dropzone p {
      margin: 8px 0 0;
      font-size: 14px;
    }
    .file-input {
      position: absolute;
      inset: 0;
      opacity: 0;
      cursor: pointer;
    }
    .upload-icon {
      color: var(--primary-color, #0288d1);
      opacity: 0.85;
    }
    .upload-tip {
      margin: 8px 0 0;
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.45));
    }
    .pick-list {
      list-style: none;
      margin: 12px 0 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 6px;
      max-height: 120px;
      overflow: auto;
    }
    .pick-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 8px;
      padding: 6px 10px;
      border-radius: 8px;
      background: var(--secondary-background-color, #f7f9fb);
      font-size: 13px;
    }
    .pick-item.success {
      color: #2e7d32;
    }
    .pick-item.fail {
      color: #c62828;
    }
    .link-btn {
      border: none;
      background: none;
      color: var(--primary-color, #0288d1);
      cursor: pointer;
      font-size: 12px;
      padding: 0;
    }
    .actions {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 12px;
      margin-top: 12px;
    }
    .status {
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.55));
    }
    .progress-wrap {
      margin-top: 10px;
    }
    .progress-bar {
      height: 6px;
      border-radius: 999px;
      background: rgba(0, 0, 0, 0.08);
      overflow: hidden;
    }
    .progress-fill {
      height: 100%;
      background: var(--primary-color, #0288d1);
      transition: width 0.2s;
    }
    .progress-label {
      display: block;
      margin-top: 4px;
      font-size: 12px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.5));
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
    .file-groups {
      flex: 1;
      min-height: 0;
      overflow: auto;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .file-group {
      flex-shrink: 0;
    }
    .file-list {
      list-style: none;
      margin: 0;
      padding: 0;
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
    "flow-media-panel": FlowMediaPanel;
  }
}
