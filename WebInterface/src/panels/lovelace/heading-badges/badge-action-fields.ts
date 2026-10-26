import { html, nothing, type TemplateResult } from "lit";
import type { BadgeTapActionConfig, LovelaceCardConfig } from "../../../types";
import { getCardEntry, getPickerCards } from "../../../registry/card-registry";
import { formatBadgeWriteValue } from "./badge-value";
import {
  normalizeTapAction,
  parseBadgeWriteValue,
  type NormalizedTapActionKind,
  type RawBadgeConfig,
} from "./badge-model";
import "../editor/flow-card-config-editor";

export interface BadgeActionFieldsPatch {
  tap_action?: BadgeTapActionConfig;
  /** 同步扁平镜像，便于旧逻辑读取 */
  action?: NormalizedTapActionKind;
  press_value?: ReturnType<typeof parseBadgeWriteValue>;
  release_value?: ReturnType<typeof parseBadgeWriteValue>;
  card?: LovelaceCardConfig;
  /**
   * open-card 时把卡片地址同步到徽章 entity，便于列表展示；
   * 非轨迹场景徽章本身不再单独配置读写地址。
   */
  entity?: string;
}

/**
 * 弹窗内不适合再嵌套的容器卡（自身含徽章/复杂布局）。
 * 其余注册卡片均可通过 getOpenCardOptions() 选出。
 */
const OPEN_CARD_EXCLUDE = new Set([
  "heading",
  "picture-elements",
  "path-badge",
  "error",
  "color", // 旧别名，用 rgba
]);

/** 控制类优先，其余按注册名排序，保证下拉稳定。 */
const OPEN_CARD_PRIORITY = [
  "slider",
  "climate",
  "cover",
  "switch",
  "tile",
  "gain",
  "multi-fader",
  "xy-pad",
  "trigger",
  "media",
  "mic",
  "rgba",
  "hsv",
  "sensor",
  "label",
  "status",
  "clock",
  "bar",
  "line2d",
  "scatter2d",
  "line3d",
  "scatter3d",
  "markdown",
  "link",
];

function getOpenCardOptions(): Array<{ type: string; label: string }> {
  const entries = getPickerCards().filter((c) => !OPEN_CARD_EXCLUDE.has(c.type));
  const priority = new Map(OPEN_CARD_PRIORITY.map((type, i) => [type, i]));
  entries.sort((a, b) => {
    const pa = priority.get(a.type) ?? 1000;
    const pb = priority.get(b.type) ?? 1000;
    if (pa !== pb) return pa - pb;
    return a.name.localeCompare(b.name, "zh");
  });
  return entries.map((c) => ({ type: c.type, label: c.name }));
}

function stubOpenCard(
  type: string,
  entity: string,
  prev?: LovelaceCardConfig,
): LovelaceCardConfig {
  const entry = getCardEntry(type);
  const stub = entry?.constructor?.getStubConfig?.() ?? ({ type } as LovelaceCardConfig);
  const nextEntity =
    entity || String(prev?.entity ?? stub.entity ?? "").trim();
  // 同类型：保留已编辑的范围/样式；换类型：用 stub 并继承地址
  if (prev && prev.type === type) {
    return {
      ...prev,
      type,
      ...(nextEntity ? { entity: nextEntity } : {}),
    };
  }
  return {
    ...stub,
    type,
    ...(nextEntity ? { entity: nextEntity } : {}),
  };
}

function emitAction(
  kind: NormalizedTapActionKind,
  fields: Partial<BadgeTapActionConfig>,
  patch: (partial: BadgeActionFieldsPatch) => void,
  extras?: Pick<BadgeActionFieldsPatch, "entity">,
): void {
  const tap_action: BadgeTapActionConfig = { action: kind, ...fields };
  const next: BadgeActionFieldsPatch = {
    tap_action,
    action: kind,
    press_value: tap_action.press_value,
    release_value: tap_action.release_value,
    card: tap_action.card,
  };
  // 弹出卡片：读写只认卡片配置里的地址，同步到徽章 entity 仅作展示/回退
  if (kind === "open-card") {
    const cardEntity = String(tap_action.card?.entity ?? extras?.entity ?? "").trim();
    if (cardEntity) next.entity = cardEntity;
  }
  if (extras?.entity !== undefined && kind !== "open-card") {
    next.entity = extras.entity;
  }
  patch(next);
}

/** 控制徽章共用的 tap_action 表单。 */
export function renderBadgeActionFields(
  config: unknown,
  patch: (partial: BadgeActionFieldsPatch) => void,
): TemplateResult {
  const raw = (config ?? {}) as RawBadgeConfig;
  const tap = normalizeTapAction(raw);
  const badgeEntity = String(raw.entity ?? "").trim();
  const action = tap.action;
  const openCard = tap.card ?? stubOpenCard("slider", badgeEntity);
  const openCardType = String(openCard.type ?? "slider").trim() || "slider";
  const openCardOptions = getOpenCardOptions();

  return html`
    <label class="field">
      <span>按下行为</span>
      <select
        @change=${(ev: Event) => {
          const next = (ev.target as HTMLSelectElement).value as NormalizedTapActionKind;
          if (next === "switch") {
            emitAction(next, {}, patch);
            return;
          }
          if (next === "rw") {
            emitAction(
              next,
              {
                press_value: tap.press_value ?? true,
                release_value: tap.release_value ?? false,
              },
              patch,
            );
            return;
          }
          if (next === "open-card") {
            const card = stubOpenCard(openCardType, badgeEntity, openCard);
            emitAction(next, { card }, patch, {
              entity: String(card.entity ?? badgeEntity).trim(),
            });
            return;
          }
          emitAction("trigger", {}, patch);
        }}
      >
        <option value="trigger" ?selected=${action === "trigger"}>触发（自动弹起）</option>
        <option value="switch" ?selected=${action === "switch"}>开关（保持按下）</option>
        <option value="rw" ?selected=${action === "rw"}>读写（保持按下）</option>
        <option value="open-card" ?selected=${action === "open-card"}>弹出卡片</option>
      </select>
      <p class="helper">
        ${action === "trigger"
          ? "单击发送 true，随后自动弹起。读写地址请填「绑定地址」。"
          : action === "switch"
            ? "单击切换 true/false；跟随实体状态。读写地址请填「绑定地址」。"
            : action === "rw"
              ? "单击切换按下/弹起值；接收值匹配时同步外观。读写地址请填「绑定地址」。"
              : action === "open-card"
                ? "单击只弹出卡片，不向徽章写值。读写地址请在下方卡片设置中配置。"
                : ""}
      </p>
    </label>
    ${action === "rw"
      ? html`
          <label class="field">
            <span>按下发送值</span>
            <input
              .value=${formatBadgeWriteValue(tap.press_value, true)}
              placeholder="true"
              @input=${(ev: Event) =>
                emitAction(
                  "rw",
                  {
                    press_value: parseBadgeWriteValue((ev.target as HTMLInputElement).value),
                    release_value: tap.release_value ?? false,
                  },
                  patch,
                )}
            />
          </label>
          <label class="field">
            <span>弹起发送值</span>
            <input
              .value=${formatBadgeWriteValue(tap.release_value, false)}
              placeholder="false"
              @input=${(ev: Event) =>
                emitAction(
                  "rw",
                  {
                    press_value: tap.press_value ?? true,
                    release_value: parseBadgeWriteValue((ev.target as HTMLInputElement).value),
                  },
                  patch,
                )}
            />
            <p class="helper">
              再点一次发送弹起值。若实体当前值等于按下或弹起值，徽章状态会同步更新。
            </p>
          </label>
        `
      : nothing}
    ${action === "open-card"
      ? html`
          <label class="field">
            <span>弹出卡片类型</span>
            <select
              @change=${(ev: Event) => {
                const type = (ev.target as HTMLSelectElement).value;
                const card = stubOpenCard(type, badgeEntity, openCard);
                emitAction("open-card", { card }, patch, {
                  entity: String(card.entity ?? "").trim(),
                });
              }}
            >
              ${openCardOptions.map(
                (opt) => html`
                  <option value=${opt.type} ?selected=${openCardType === opt.type}>
                    ${opt.label}
                  </option>
                `,
              )}
              ${!openCardOptions.some((o) => o.type === openCardType)
                ? html`<option value=${openCardType} selected>${openCardType}</option>`
                : nothing}
            </select>
          </label>
          <p class="helper">卡片的读写地址、范围与样式只在下方配置，与徽章绑定地址无关。</p>
          <div
            class="open-card-editor"
            @config-changed=${(ev: Event) => {
              ev.stopPropagation();
              const detail = (ev as CustomEvent<{ config: LovelaceCardConfig }>).detail;
              if (!detail?.config) return;
              emitAction("open-card", { card: detail.config }, patch, {
                entity: String(detail.config.entity ?? "").trim(),
              });
            }}
          >
            <flow-card-config-editor .config=${openCard}></flow-card-config-editor>
          </div>
        `
      : nothing}
  `;
}
