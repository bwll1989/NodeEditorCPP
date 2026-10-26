import type {
  BadgeTapActionConfig,
  FlowValue,
  LovelaceCardConfig,
  LovelaceHeadingBadgeConfig,
  LovelacePathBadgeConfig,
  LovelacePictureElementConfig,
} from "../../../types";

/** Parse editor / config text into a write value. */
export function parseBadgeWriteValue(raw: unknown): FlowValue {
  if (typeof raw === "boolean" || typeof raw === "number") return raw;
  if (raw === null || raw === undefined) return true;
  const text = String(raw).trim();
  if (text === "") return true;
  if (text === "true") return true;
  if (text === "false") return false;
  const num = Number(text);
  if (text !== "" && Number.isFinite(num) && String(num) === text) return num;
  return text;
}

/** 任意徽章 / 图像元素原始配置（含旧字段）。 */
export type RawBadgeConfig = {
  type?: string;
  entity?: string;
  name?: string;
  icon?: string;
  color?: string;
  unit?: string;
  text?: string;
  show_icon?: boolean;
  show_name?: boolean;
  show_state?: boolean;
  show_background?: boolean;
  size?: number;
  attribute?: string;
  style?: LovelacePictureElementConfig["style"];
  tap_action?: BadgeTapActionConfig;
  action?: string;
  value?: FlowValue;
  press_value?: FlowValue;
  release_value?: FlowValue;
  card?: LovelaceCardConfig;
  [key: string]: unknown;
};

export type BadgeRole = "entity" | "button";

/** 归一化后的动作种类（不含 deprecated 别名）。 */
export type NormalizedTapActionKind =
  | "none"
  | "trigger"
  | "switch"
  | "rw"
  | "open-card";

export interface NormalizedBadgeConfig {
  type: BadgeRole;
  entity?: string;
  name?: string;
  icon?: string;
  color?: string;
  unit?: string;
  text?: string;
  show_icon?: boolean;
  show_name?: boolean;
  show_state?: boolean;
  show_background?: boolean;
  size?: number;
  attribute?: string;
  tap_action: BadgeTapActionConfig & { action: NormalizedTapActionKind };
  style?: LovelacePictureElementConfig["style"];
}

function asRecord(raw: unknown): RawBadgeConfig {
  return raw && typeof raw === "object" ? (raw as RawBadgeConfig) : ({} as RawBadgeConfig);
}

function valuesLooselyEqual(a: unknown, b: unknown): boolean {
  if (a === b) return true;
  if (a === undefined || b === undefined || a === null || b === null) return false;
  if (typeof a === "boolean" || typeof b === "boolean") {
    const toBool = (v: unknown): boolean | undefined => {
      if (typeof v === "boolean") return v;
      if (typeof v === "number") return v !== 0;
      const s = String(v).trim().toLowerCase();
      if (s === "true" || s === "on" || s === "1") return true;
      if (s === "false" || s === "off" || s === "0") return false;
      return undefined;
    };
    const ba = toBool(a);
    const bb = toBool(b);
    return ba !== undefined && bb !== undefined && ba === bb;
  }
  if (typeof a === "number" || typeof b === "number") {
    const na = Number(a);
    const nb = Number(b);
    return Number.isFinite(na) && Number.isFinite(nb) && na === nb;
  }
  return String(a) === String(b);
}

export function badgeValuesEqual(a: unknown, b: unknown): boolean {
  return valuesLooselyEqual(a, b);
}

/** 旧 type: state-badge / entity → entity；button → button。 */
export function badgeRole(type?: string): BadgeRole {
  return String(type ?? "entity") === "button" ? "button" : "entity";
}

export function isButtonBadge(config?: { type?: string }): boolean {
  return badgeRole(config?.type) === "button";
}

export function isEntityBadge(config?: { type?: string }): boolean {
  return !isButtonBadge(config);
}

export function isLatchBadgeAction(action?: string): boolean {
  return action === "switch" || action === "rw" || action === "toggle";
}

/**
 * 归一化动作种类；兼容旧 toggle/set。
 * toggle + 非默认 press/release → rw；否则 → switch。
 * set → trigger。
 */
export function normalizeTapActionKind(
  raw?: string,
  press?: unknown,
  release?: unknown,
): NormalizedTapActionKind {
  const value = String(raw ?? "trigger");
  if (value === "none") return "none";
  if (value === "open-card") return "open-card";
  if (value === "switch") return "switch";
  if (value === "rw") return "rw";
  if (value === "set") return "trigger";
  if (value === "toggle") {
    const hasCustom =
      (press !== undefined && !valuesLooselyEqual(press, true)) ||
      (release !== undefined && !valuesLooselyEqual(release, false));
    return hasCustom ? "rw" : "switch";
  }
  if (value === "trigger") return "trigger";
  return "trigger";
}

function inputCard(raw: RawBadgeConfig): LovelaceCardConfig | undefined {
  const card = raw.card;
  return card && typeof card === "object" ? card : undefined;
}

/**
 * 从旧扁平字段或新 tap_action 解析动作配置。
 * 兼容：顶层 action/value/press_value/release_value/card，以及 toggle/set。
 */
export function normalizeTapAction(raw: RawBadgeConfig): BadgeTapActionConfig & {
  action: NormalizedTapActionKind;
} {
  const nested = raw.tap_action && typeof raw.tap_action === "object" ? raw.tap_action : undefined;
  const press =
    nested?.press_value !== undefined
      ? nested.press_value
      : raw.press_value !== undefined
        ? raw.press_value
        : undefined;
  const release =
    nested?.release_value !== undefined
      ? nested.release_value
      : raw.release_value !== undefined
        ? raw.release_value
        : undefined;

  const kind = normalizeTapActionKind(
    nested?.action ?? (typeof raw.action === "string" ? raw.action : undefined),
    press,
    release,
  );

  if (kind === "none") return { action: "none" };

  if (kind === "switch") {
    return { action: "switch" };
  }

  if (kind === "rw") {
    return {
      action: "rw",
      press_value: press === undefined ? true : parseBadgeWriteValue(press),
      release_value: release === undefined ? false : parseBadgeWriteValue(release),
    };
  }

  if (kind === "open-card") {
    const cardRaw =
      nested?.card && typeof nested.card === "object" ? nested.card : inputCard(raw);
    const cardType = String(cardRaw?.type ?? "slider").trim() || "slider";
    const cardEntity =
      String(cardRaw?.entity ?? raw.entity ?? "").trim() || String(raw.entity ?? "").trim();
    return {
      action: "open-card",
      card: {
        ...(cardRaw && typeof cardRaw === "object" ? cardRaw : {}),
        type: cardType,
        ...(cardEntity ? { entity: cardEntity } : {}),
      },
    };
  }

  // trigger：固定发 true，不保留自定义 value
  return { action: "trigger" };
}

/** 读取时归一化：统一 type，补齐 tap_action。 */
export function normalizeBadgeConfig(raw: unknown): NormalizedBadgeConfig {
  const input = asRecord(raw);
  const type = badgeRole(input.type);
  const tap_action =
    type === "button" ? normalizeTapAction(input) : { action: "none" as const };

  const config: NormalizedBadgeConfig = {
    type,
    tap_action,
  };

  const entity = String(input.entity ?? "").trim();
  if (entity) config.entity = entity;

  const name = String(input.name ?? "").trim();
  if (name) config.name = name;

  const icon = String(input.icon ?? "").trim();
  if (icon) config.icon = icon;

  const color = String(input.color ?? "").trim();
  if (color) config.color = color;

  const unit = String(input.unit ?? "").trim();
  if (unit) config.unit = unit;

  const text = String(input.text ?? "").trim();
  if (text) config.text = text;

  if (input.show_icon === false) config.show_icon = false;
  if (typeof input.show_name === "boolean") config.show_name = input.show_name;
  if (typeof input.show_state === "boolean") config.show_state = input.show_state;
  if (input.show_background === false) config.show_background = false;

  const size = Number(input.size);
  if (Number.isFinite(size) && size > 0) config.size = size;

  const attribute = String(input.attribute ?? "").trim();
  if (attribute) config.attribute = attribute;

  if (input.style && typeof input.style === "object") {
    config.style = input.style as LovelacePictureElementConfig["style"];
  }

  return config;
}

/**
 * 写出配置：新结构 + 旧扁平镜像，保证旧逻辑/旧文件可读。
 */
export function serializeBadgeConfig(
  config: Partial<NormalizedBadgeConfig> | RawBadgeConfig | NormalizedBadgeConfig,
): LovelaceHeadingBadgeConfig &
  LovelacePictureElementConfig &
  Partial<LovelacePathBadgeConfig> {
  const normalized = normalizeBadgeConfig(config);
  const tap = normalized.tap_action;

  const out: Record<string, unknown> = {
    type: normalized.type,
  };

  if (normalized.entity) out.entity = normalized.entity;
  if (normalized.name) out.name = normalized.name;
  if (normalized.icon) out.icon = normalized.icon;
  if (normalized.color) out.color = normalized.color;
  if (normalized.unit) out.unit = normalized.unit;
  if (normalized.text) out.text = normalized.text;
  if (normalized.show_icon === false) out.show_icon = false;
  if (typeof normalized.show_name === "boolean") out.show_name = normalized.show_name;
  if (typeof normalized.show_state === "boolean") out.show_state = normalized.show_state;
  if (normalized.show_background === false) out.show_background = false;
  if (normalized.size !== undefined) out.size = normalized.size;
  if (normalized.attribute) out.attribute = normalized.attribute;
  if (normalized.style) out.style = normalized.style;

  if (normalized.type === "button") {
    const cleanTap: BadgeTapActionConfig = { action: tap.action };
    if (tap.action === "rw") {
      cleanTap.press_value = tap.press_value;
      cleanTap.release_value = tap.release_value;
    }
    if (tap.action === "open-card" && tap.card) {
      cleanTap.card = tap.card;
    }
    out.tap_action = cleanTap;
    out.action = tap.action;
    if (tap.press_value !== undefined) out.press_value = tap.press_value;
    if (tap.release_value !== undefined) out.release_value = tap.release_value;
    if (tap.card) out.card = tap.card;
  }

  return out as unknown as LovelaceHeadingBadgeConfig & LovelacePictureElementConfig;
}

export function createEntityBadgeStub(
  extras?: Partial<NormalizedBadgeConfig>,
): ReturnType<typeof serializeBadgeConfig> {
  return serializeBadgeConfig({
    type: "entity",
    entity: "",
    icon: "mdi:thermometer",
    show_icon: true,
    show_name: true,
    ...extras,
  });
}

export function createButtonBadgeStub(
  extras?: Partial<NormalizedBadgeConfig>,
): ReturnType<typeof serializeBadgeConfig> {
  return serializeBadgeConfig({
    type: "button",
    entity: "",
    icon: "mdi:gesture-tap-button",
    text: "触发",
    show_name: true,
    tap_action: { action: "trigger" },
    ...extras,
  });
}

export function patchBadgeConfig(
  current: unknown,
  patch: Partial<NormalizedBadgeConfig> | RawBadgeConfig | Record<string, unknown>,
): ReturnType<typeof serializeBadgeConfig> {
  const base = normalizeBadgeConfig(current);
  const patchRec = asRecord(patch);
  const nextTap =
    patchRec.tap_action ||
    patchRec.action ||
    patchRec.value !== undefined ||
    patchRec.press_value !== undefined ||
    patchRec.release_value !== undefined ||
    patchRec.card !== undefined
      ? normalizeTapAction({
          ...base,
          ...patchRec,
          tap_action: patchRec.tap_action ?? base.tap_action,
        })
      : base.tap_action;

  return serializeBadgeConfig({
    ...base,
    ...patchRec,
    tap_action: nextTap,
  });
}
