import type { Flow, FlowEntityState, FlowValue, LovelaceCardConfig } from "../../../types";
import { stateActive } from "../../../common/entity/state-active";
import { showBadgeCardDialog } from "../../../dialogs/show-badge-card-dialog";
import {
  badgeValuesEqual,
  isLatchBadgeAction,
  normalizeBadgeConfig,
  parseBadgeWriteValue,
  type NormalizedBadgeConfig,
} from "./badge-model";

export interface BadgeActionContext {
  /** switch / rw 乐观按下态；undefined 时跟随实体 */
  latched?: boolean;
  preview?: boolean;
}

export type BadgeActionResult =
  | { mode: "none" }
  | { mode: "pulse" }
  | { mode: "latch"; pressed: boolean };

function rwPressValue(config: NormalizedBadgeConfig): FlowValue {
  return config.tap_action.press_value === undefined
    ? true
    : parseBadgeWriteValue(config.tap_action.press_value);
}

function rwReleaseValue(config: NormalizedBadgeConfig): FlowValue {
  return config.tap_action.release_value === undefined
    ? false
    : parseBadgeWriteValue(config.tap_action.release_value);
}

/**
 * 从实体状态推断 latch 按下态。
 * switch：stateActive；rw：值等于 press/release 才确定，否则 undefined（保持乐观态）。
 */
export function resolveLatchPressedFromState(
  rawConfig: unknown,
  state: FlowEntityState | undefined,
): boolean | undefined {
  const config = normalizeBadgeConfig(rawConfig);
  const action = config.tap_action.action;
  if (action === "switch") {
    return stateActive(state);
  }
  if (action === "rw") {
    if (!state) return undefined;
    const value = state.state;
    const press = rwPressValue(config);
    const release = rwReleaseValue(config);
    if (badgeValuesEqual(value, press)) return true;
    if (badgeValuesEqual(value, release)) return false;
    return undefined;
  }
  return undefined;
}

function isPressed(
  rawConfig: unknown,
  state: FlowEntityState | undefined,
  latched?: boolean,
): boolean {
  if (typeof latched === "boolean") return latched;
  const resolved = resolveLatchPressedFromState(rawConfig, state);
  if (typeof resolved === "boolean") return resolved;
  return false;
}

function resolveOpenCardConfig(
  config: NormalizedBadgeConfig,
): LovelaceCardConfig | undefined {
  const card = config.tap_action.card;
  if (!card || typeof card !== "object") return undefined;
  const badgeEntity = String(config.entity ?? "").trim();
  const cardEntity = String(card.entity ?? "").trim() || badgeEntity;
  if (!String(card.type ?? "").trim()) return undefined;
  return {
    ...card,
    type: String(card.type).trim(),
    ...(cardEntity ? { entity: cardEntity } : {}),
    ...(config.name && !card.name ? { name: config.name } : {}),
    ...(config.icon && !card.icon ? { icon: config.icon } : {}),
  };
}

/**
 * 统一徽章点击执行器。
 * 卡片 / 标题行 / 图像 / 轨迹控制徽章都走这里，组件只负责外观与乐观态。
 */
export async function handleBadgeTap(
  flow: Flow | undefined,
  rawConfig: unknown,
  ctx: BadgeActionContext = {},
): Promise<BadgeActionResult> {
  if (!flow || !rawConfig || ctx.preview) return { mode: "none" };

  const config = normalizeBadgeConfig(rawConfig);
  if (config.type !== "button") return { mode: "none" };

  const action = config.tap_action.action;
  if (action === "none") return { mode: "none" };

  if (action === "open-card") {
    const cardConfig = resolveOpenCardConfig(config);
    if (!cardConfig) return { mode: "none" };
    const title = String(config.text ?? config.name ?? cardConfig.name ?? "").trim();
    void showBadgeCardDialog({ flow, cardConfig, title });
    return { mode: "none" };
  }

  const entity = String(config.entity ?? "").trim();
  if (!entity) return { mode: "none" };

  if (action === "switch") {
    const state = flow.states[entity];
    const pressed = isPressed(rawConfig, state, ctx.latched);
    const next = !pressed;
    await flow.callService(entity, next);
    return { mode: "latch", pressed: next };
  }

  if (action === "rw") {
    const state = flow.states[entity];
    const pressed = isPressed(rawConfig, state, ctx.latched);
    const next = !pressed;
    await flow.callService(entity, next ? rwPressValue(config) : rwReleaseValue(config));
    return { mode: "latch", pressed: next };
  }

  // trigger：固定发 true，UI 自动弹起
  await flow.callService(entity, true);
  return { mode: "pulse" };
}

/** 控制徽章当前是否应显示为按下。 */
export function badgePressedVisual(
  rawConfig: unknown,
  state: FlowEntityState | undefined,
  opts: { flashing?: boolean; latched?: boolean } = {},
): boolean {
  const config = normalizeBadgeConfig(rawConfig);
  if (config.type !== "button") return false;
  if (isLatchBadgeAction(config.tap_action.action)) {
    return isPressed(rawConfig, state, opts.latched);
  }
  return Boolean(opts.flashing);
}

/** 徽章是否可点击（open-card 可不绑徽章地址）。 */
export function badgeTapEnabled(rawConfig: unknown, preview?: boolean): boolean {
  if (preview) return false;
  const config = normalizeBadgeConfig(rawConfig);
  if (config.type !== "button") return false;
  if (config.tap_action.action === "open-card") {
    return Boolean(config.tap_action.card?.type);
  }
  return Boolean(String(config.entity ?? "").trim());
}
