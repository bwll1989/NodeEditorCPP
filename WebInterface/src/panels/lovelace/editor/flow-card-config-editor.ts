import { css, html, LitElement, nothing, type TemplateResult } from "lit";
import { customElement, property } from "lit/decorators.js";
import type {
  ConfigFieldSchema,
  LovelaceBarEntityConfig,
  LovelaceCardConfig,
  LovelaceHeadingBadgeConfig,
  LovelacePictureElementConfig,
  LovelaceStatusEntityConfig,
} from "../../../types";
import type { EntityPickerValueChangedDetail } from "../../../components/ha-entity-picker";
import {
  denormalizeCardConfig,
  getCardConfigSchema,
  normalizeCardConfig,
} from "./card-config-schemas";
import "../../../components/ha-expandable-panel";
import "../../../components/ha-entity-picker";
import "../../../components/ha-icon-picker";
import "../../../components/ha-color-picker";
import "../../../components/ha-select-box";
import "./flow-heading-badges-editor";
import "./flow-bar-entities-editor";
import "./flow-status-entities-editor";
import "./flow-picture-elements-editor";
import { DEFAULT_FLOORPLAN } from "../picture-elements/default-floorplan";

@customElement("flow-card-config-editor")
export class FlowCardConfigEditor extends LitElement {
  @property({ attribute: false }) public config: LovelaceCardConfig = { type: "tile" };

  private _data(): Record<string, unknown> {
    return denormalizeCardConfig(this.config as Record<string, unknown>);
  }

  private _isVisible(schema: ConfigFieldSchema, data: Record<string, unknown>): boolean {
    if (!schema.visible) return true;
    const current = data[schema.visible.field];
    if (schema.visible.operator === "eq") return current === schema.visible.value;
    return current !== schema.visible.value;
  }

  private _emit(patch: Record<string, unknown>): void {
    const layoutChanged = "content_layout" in patch;
    const editorData = { ...this._data(), ...patch };
    const normalized = normalizeCardConfig(editorData, { layoutChanged });
    const value: LovelaceCardConfig = {
      ...this.config,
      ...normalized,
    } as LovelaceCardConfig;

    // normalizeCardConfig removes text_only via delete; spread above would keep stale true.
    if (!normalized.text_only) {
      delete (value as Record<string, unknown>).text_only;
    }

    // Same for status list_columns: default single column omits the key.
    if (Number(normalized.list_columns) !== 2) {
      delete (value as Record<string, unknown>).list_columns;
    }

    this.dispatchEvent(
      new CustomEvent("config-changed", {
        bubbles: true,
        composed: true,
        detail: { config: value },
      }),
    );
  }

  private _setField(name: string, value: unknown): void {
    if (!name) return;
    this._emit({ [name]: value });
  }

  applyPictureElementPosition(left: number, top: number): boolean {
    const editor = this.renderRoot.querySelector("flow-picture-elements-editor") as
      | import("./flow-picture-elements-editor").FlowPictureElementsEditor
      | null;
    if (!editor || editor.selectedIndex < 0) return false;
    editor.applyPosition(left, top);
    return true;
  }

  private _renderField(schema: ConfigFieldSchema, data: Record<string, unknown>): TemplateResult | typeof nothing {
    if (!this._isVisible(schema, data)) return nothing;

    switch (schema.type) {
      case "section":
        return html`
          <ha-expandable-panel .header=${schema.label}>
            ${schema.schema?.map((child) => this._renderField(child, data))}
          </ha-expandable-panel>
        `;

      case "grid": {
        const isAppearance = schema.gridVariant === "appearance";
        return html`
          <div class="grid ${isAppearance ? "appearance" : ""}">
            ${schema.schema?.map((child) => this._renderField(child, data))}
          </div>
          ${isAppearance
            ? html`<p class="appearance-helper">非活动状态（例如关闭或闭合）将不会被着色。</p>`
            : nothing}
        `;
      }

      case "entity":
        return html`
          <ha-entity-picker
            .label=${schema.label}
            .value=${String(data[schema.name] ?? "")}
            .placeholder=${schema.placeholder ?? ""}
            ?optional=${schema.optional}
            @value-changed=${(ev: CustomEvent<EntityPickerValueChangedDetail>) => {
              const patch: Record<string, unknown> = { [schema.name]: ev.detail.value };
              if (ev.detail.actionName && schema.name === "entity") {
                patch.name = ev.detail.actionName;
              }
              this._emit(patch);
            }}
          ></ha-entity-picker>
          ${schema.helper ? html`<p class="helper">${schema.helper}</p>` : nothing}
        `;

      case "icon":
        return html`
          <ha-icon-picker
            .label=${schema.label}
            .value=${String(data[schema.name] ?? "")}
            .placeholder=${schema.placeholder ?? "mdi:home"}
            ?compact=${schema.compact}
            @icon-changed=${(ev: CustomEvent<{ value: string }>) =>
              this._setField(schema.name, ev.detail.value)}
          ></ha-icon-picker>
        `;

      case "color":
        return html`
          <ha-color-picker
            .label=${schema.label}
            .value=${String(data[schema.name] ?? "")}
            ?compact=${schema.compact}
            ?hideHelper=${schema.compact}
            @color-changed=${(ev: CustomEvent<{ value: string }>) =>
              this._setField(schema.name, ev.detail.value)}
          ></ha-color-picker>
        `;

      case "boolean":
        return html`
          <label class="switch-row">
            <div>
              <div class="switch-label">${schema.label}</div>
              ${schema.helper ? html`<div class="helper">${schema.helper}</div>` : nothing}
            </div>
            <input
              type="checkbox"
              .checked=${Boolean(data[schema.name])}
              @change=${(ev: Event) =>
                this._setField(schema.name, (ev.target as HTMLInputElement).checked)}
            />
          </label>
        `;

      case "select":
        return html`
          <label class="field">
            <span class="label-text">
              ${schema.label}
              ${schema.optional ? html`<span class="optional">（可选）</span>` : nothing}
            </span>
            <select
              @change=${(ev: Event) =>
                this._setField(schema.name, (ev.target as HTMLSelectElement).value)}
            >
              ${schema.options?.map((opt) => {
                const current = String(
                  data[schema.name] ?? schema.options?.[0]?.value ?? "",
                );
                return html`
                  <option
                    value=${opt.value}
                    ?selected=${current === opt.value}
                    title=${opt.description ?? ""}
                  >
                    ${opt.label}
                  </option>
                `;
              })}
            </select>
            ${schema.helper ? html`<p class="helper">${schema.helper}</p>` : nothing}
          </label>
        `;

      case "select_box": {
        const current = String(
          data[schema.name] ?? schema.options?.[0]?.value ?? "",
        );
        return html`
          <ha-select-box
            .label=${schema.label}
            .value=${current}
            .options=${schema.options ?? []}
            .maxColumns=${schema.maxColumns ?? 2}
            @value-changed=${(ev: CustomEvent<{ value: string }>) => {
              ev.stopPropagation();
              this._setField(schema.name, ev.detail.value);
            }}
          ></ha-select-box>
          ${schema.helper ? html`<p class="helper">${schema.helper}</p>` : nothing}
        `;
      }

      case "textarea":
        return html`
          <label class="field">
            <span class="label-text">${schema.label}</span>
            <textarea
              rows="6"
              .value=${String(data[schema.name] ?? "")}
              @input=${(ev: Event) =>
                this._setField(schema.name, (ev.target as HTMLTextAreaElement).value)}
            ></textarea>
            ${schema.helper ? html`<p class="helper">${schema.helper}</p>` : nothing}
          </label>
        `;

      case "number":
        return html`
          <label class="field">
            <span class="label-text">${schema.label}</span>
            <input
              type="number"
              .value=${String(data[schema.name] ?? "")}
              @input=${(ev: Event) =>
                this._setField(
                  schema.name,
                  Number((ev.target as HTMLInputElement).value) || 0,
                )}
            />
          </label>
        `;

      case "badges":
        return html`
          <flow-heading-badges-editor
              .badges=${(data.badges as LovelaceHeadingBadgeConfig[]) ?? []}
            @badges-changed=${(ev: CustomEvent<{ badges: LovelaceHeadingBadgeConfig[] }>) => {
              ev.stopPropagation();
              this._setField("badges", ev.detail.badges);
            }}
          ></flow-heading-badges-editor>
        `;

      case "bar_entities":
        return html`
          <flow-bar-entities-editor
            .entities=${(data.entities as LovelaceBarEntityConfig[]) ?? []}
            @entities-changed=${(ev: CustomEvent<{ entities: LovelaceBarEntityConfig[] }>) => {
              ev.stopPropagation();
              this._setField("entities", ev.detail.entities);
            }}
          ></flow-bar-entities-editor>
        `;

      case "status_entities":
        return html`
          <flow-status-entities-editor
            .entities=${(data.entities as LovelaceStatusEntityConfig[]) ?? []}
            @entities-changed=${(ev: CustomEvent<{ entities: LovelaceStatusEntityConfig[] }>) => {
              ev.stopPropagation();
              this._setField("entities", ev.detail.entities);
            }}
          ></flow-status-entities-editor>
        `;

      case "image":
        return html`
          <label class="field">
            <span class="label-text">${schema.label}</span>
            <input
              type="text"
              .value=${String(data[schema.name] ?? "")}
              placeholder="https://… 或选择本地图片"
              @input=${(ev: Event) =>
                this._setField(schema.name, (ev.target as HTMLInputElement).value)}
            />
            <div class="image-actions">
              <label class="file-btn">
                上传图片
                <input
                  type="file"
                  accept="image/*"
                  hidden
                  @change=${(ev: Event) => {
                    const file = (ev.target as HTMLInputElement).files?.[0];
                    (ev.target as HTMLInputElement).value = "";
                    if (!file) return;
                    const reader = new FileReader();
                    reader.onload = () => this._setField(schema.name, String(reader.result ?? ""));
                    reader.readAsDataURL(file);
                  }}
                />
              </label>
              <button
                type="button"
                class="link-btn"
                @click=${() => this._setField(schema.name, DEFAULT_FLOORPLAN)}
              >
                使用默认户型图
              </button>
            </div>
            ${schema.helper ? html`<p class="helper">${schema.helper}</p>` : nothing}
          </label>
        `;

      case "picture_elements":
        return html`
          <flow-picture-elements-editor
            .elements=${(data.elements as LovelacePictureElementConfig[]) ?? []}
            @elements-changed=${(ev: CustomEvent<{ elements: LovelacePictureElementConfig[] }>) => {
              ev.stopPropagation();
              this._setField("elements", ev.detail.elements);
            }}
          ></flow-picture-elements-editor>
        `;

      case "text":
      default:
        return html`
          <label class="field">
            <span class="label-text">
              ${schema.label}
              ${schema.optional ? html`<span class="optional">（可选）</span>` : nothing}
            </span>
            <input
              type="text"
              .value=${String(data[schema.name] ?? "")}
              placeholder=${schema.placeholder ?? ""}
              @input=${(ev: Event) =>
                this._setField(schema.name, (ev.target as HTMLInputElement).value)}
            />
            ${schema.helper ? html`<p class="helper">${schema.helper}</p>` : nothing}
          </label>
        `;
    }
  }

  protected render() {
    const type = String(this.config.type ?? "tile");
    const schema = getCardConfigSchema(type);
    const data = this._data();

    return html`
      <div class="config-editor">
        <label class="field type-field">
          <span class="label-text">类型</span>
          <input type="text" .value=${type} disabled />
        </label>
        ${schema.map((field) => this._renderField(field, data))}
      </div>
    `;
  }

  static styles = css`
    .config-editor {
      display: flex;
      flex-direction: column;
    }
    .type-field input:disabled {
      opacity: 0.7;
      background: var(--secondary-background-color, #f5f5f5);
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
    }
    .label-text {
      font-size: 13px;
      color: var(--secondary-text-color);
    }
    .optional {
      font-weight: 400;
      opacity: 0.8;
    }
    input,
    textarea,
    select {
      font: inherit;
      border: 1px solid var(--divider-color);
      border-radius: 8px;
      padding: 8px 10px;
      color: var(--primary-text-color);
      background: #fff;
    }
    textarea {
      resize: vertical;
      min-height: 120px;
      font-family: inherit;
      line-height: 1.5;
    }
    .helper {
      margin: -4px 0 8px;
      font-size: 12px;
      color: var(--secondary-text-color);
      line-height: 1.4;
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0 16px;
      align-items: start;
      margin-bottom: 12px;
    }
    .grid.appearance {
      margin-bottom: 0;
    }
    .grid > * {
      min-width: 0;
    }
    .appearance-helper {
      margin: 4px 0 12px;
      font-size: 12px;
      line-height: 1.4;
      color: var(--secondary-text-color);
    }
    @media (max-width: 520px) {
      .grid {
        grid-template-columns: 1fr;
      }
    }
    .switch-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 12px;
      cursor: pointer;
    }
    .switch-label {
      font-size: 14px;
      font-weight: 500;
      color: var(--primary-text-color);
    }
    .switch-row input[type="checkbox"] {
      width: 18px;
      height: 18px;
      accent-color: var(--primary-color);
    }
    .image-actions {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: 2px;
    }
    .file-btn,
    .link-btn {
      border: none;
      background: transparent;
      color: var(--primary-color);
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      padding: 0;
    }
    .file-btn {
      display: inline-flex;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-card-config-editor": FlowCardConfigEditor;
  }
}
