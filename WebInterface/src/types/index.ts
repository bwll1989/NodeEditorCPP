export type FlowValue = boolean | number | string | null | number[];

export interface FlowEntityState {
  address: string;
  state: FlowValue;
  attributes: Record<string, unknown>;
  last_changed: number;
}

export interface FlowConnection {
  connected: boolean;
  subscribeStates(callback: (states: Record<string, FlowEntityState>) => void): () => void;
  subscribeConnection(callback: (connected: boolean) => void): () => void;
  trackAddresses(addresses: string[]): void;
  sendCommand(address: string, value: FlowValue): void;
  query(addresses: string[]): void;
}

export interface Flow {
  connected: boolean;
  states: Record<string, FlowEntityState>;
  /** Bumps whenever any entity state changes (for Lit render dependencies). */
  readonly statesRevision: number;
  connection: FlowConnection;
  callService(address: string, value: FlowValue): Promise<void>;
  formatState(address: string): string;
  subscribeStates(callback: (states: Record<string, FlowEntityState>) => void): () => void;
  subscribeEntity(
    address: string,
    callback: (state: FlowEntityState | undefined) => void,
  ): () => void;
}

export interface LovelaceGridOptions {
  columns?: number | "full";
  rows?: number | "auto";
  min_columns?: number;
  min_rows?: number;
  max_columns?: number;
  max_rows?: number;
}

/** @deprecated use grid_options */
export interface LovelaceLayoutOptions {
  grid_columns?: number;
  grid_max_columns?: number;
  grid_min_columns?: number;
  grid_rows?: number | "auto";
  grid_max_rows?: number;
  grid_min_rows?: number;
}

export interface LovelaceCardConfig {
  type: string;
  entity?: string;
  name?: string;
  icon?: string;
  grid_options?: LovelaceGridOptions;
  layout_options?: LovelaceLayoutOptions;
  [key: string]: unknown;
}

/**
 * 控制徽章 tap 行为：
 * - trigger：触发，自动弹起，固定发 true
 * - switch：开关，保持按下，固定发 true/false，跟随实体 active
 * - rw：读写，保持按下，自定义按下/弹起值，接收值匹配时同步状态
 * - open-card：弹出与主界面相同配置的卡片
 *
 * 兼容旧值：toggle→switch（有自定义值则→rw），set→trigger。
 */
export type BadgeTapActionKind =
  | "none"
  | "trigger"
  | "switch"
  | "rw"
  | "open-card"
  /** @deprecated → switch / rw */
  | "toggle"
  /** @deprecated → trigger */
  | "set";

/** @deprecated 使用 BadgeTapActionKind */
export type BadgeControlAction = BadgeTapActionKind;

export interface BadgeTapActionConfig {
  action: BadgeTapActionKind | string;
  /** @deprecated trigger/set 旧字段，新 trigger 不再使用 */
  value?: FlowValue;
  /** rw: 按下发送值 */
  press_value?: FlowValue;
  /** rw: 弹起发送值 */
  release_value?: FlowValue;
  /** open-card: 弹出的 Lovelace 卡片完整配置（含范围/样式等） */
  card?: LovelaceCardConfig;
}

/** 标题行 / 图像 / 轨迹共用的徽章字段。 */
export interface LovelaceBadgeConfigBase {
  /** entity=状态（只读）；button=控制。旧数据 state-badge 读取时归一为 entity。 */
  type: "entity" | "button" | "state-badge" | string;
  entity?: string;
  name?: string;
  icon?: string;
  color?: string;
  unit?: string;
  text?: string;
  show_icon?: boolean;
  show_name?: boolean;
  /** @deprecated use show_name */
  show_state?: boolean;
  /** 圆形/胶囊底色；false 时透明，图标颜色跟随状态 */
  show_background?: boolean;
  size?: number;
  /** 新结构：点下去做什么 */
  tap_action?: BadgeTapActionConfig;
  /** @deprecated 扁平镜像，normalize 会并入 tap_action */
  action?: BadgeTapActionKind | string;
  /** @deprecated 扁平镜像 */
  value?: FlowValue;
  /** @deprecated 扁平镜像 */
  press_value?: FlowValue;
  /** @deprecated 扁平镜像 */
  release_value?: FlowValue;
}

export interface LovelaceHeadingBadgeConfig extends LovelaceBadgeConfigBase {}

/** Bar chart column — each entry binds one OSC / entity address. */
export interface LovelaceBarEntityConfig {
  entity: string;
  name?: string;
  color?: string;
}

/** Status card row — each entry binds one OSC / entity address. */
export interface LovelaceStatusEntityConfig {
  entity: string;
  name?: string;
  icon?: string;
}

export interface LovelacePictureElementStyle {
  top?: string | number;
  left?: string | number;
}

export interface LovelacePathPoint {
  x: number;
  y: number;
}

export interface LovelacePathBadgeConfig extends LovelaceBadgeConfigBase {
  entity: string;
  attribute?: string;
}

/** Overlay on a picture-elements card（状态 / 控制徽章）。 */
export interface LovelacePictureElementConfig extends LovelaceBadgeConfigBase {
  style?: LovelacePictureElementStyle;
}

export interface LovelaceSectionBackgroundConfig {
  color?: string;
  opacity?: number;
}

export interface LovelaceSectionConfig {
  type?: "grid";
  /** @deprecated Use heading card instead */
  title?: string;
  /** @deprecated Use heading card instead */
  icon?: string;
  /** @deprecated Use heading card badges instead */
  badges?: LovelaceBadgeConfig[];
  column_span?: number;
  row_span?: number;
  background?: boolean | LovelaceSectionBackgroundConfig;
  theme?: string;
  cards: LovelaceCardConfig[];
}

export interface LovelaceBadgeConfig {
  type: string;
  entity?: string;
  name?: string;
  icon?: string;
  [key: string]: unknown;
}

export interface LovelaceViewConfig {
  title?: string;
  path?: string;
  icon?: string;
  max_columns?: number;
  dense_section_placement?: boolean;
  sections: LovelaceSectionConfig[];
}

export interface LovelaceConfig {
  title?: string;
  views: LovelaceViewConfig[];
}

export interface LovelaceCard extends HTMLElement {
  flow?: Flow;
  preview?: boolean;
  setConfig(config: LovelaceCardConfig): void;
  getGridOptions?(): LovelaceGridOptions;
}

export interface LovelaceCardConstructor {
  new (): LovelaceCard;
  getStubConfig?(): LovelaceCardConfig;
  getConfigForm?(): CardConfigForm;
}

export interface CardConfigForm {
  schema: ConfigFieldSchema[];
}

export interface ConfigFieldSchema {
  name: string;
  label: string;
  type:
    | "text"
    | "number"
    | "boolean"
    | "select"
    | "select_box"
    | "icon"
    | "color"
    | "entity"
    | "textarea"
    | "section"
    | "grid"
    | "badges"
    | "bar_entities"
    | "status_entities"
    | "image"
    | "picture_elements"
    | "path_points"
    | "path_badges";
  helper?: string;
  placeholder?: string;
  optional?: boolean;
  compact?: boolean;
  /** Grid layout variant */
  gridVariant?: "appearance" | "default";
  /** Max columns for select_box (HA default 3) */
  maxColumns?: number;
  options?: { value: string; label: string; description?: string; image?: string }[];
  /** Nested fields for section / grid */
  schema?: ConfigFieldSchema[];
  visible?: { field: string; operator: "eq" | "not_eq"; value: unknown };
}

export interface CardRegistryEntry {
  type: string;
  name: string;
  description?: string;
  icon?: string;
  category: "tile" | "control" | "sensor" | "container";
  lazy?: boolean;
  loader?: () => Promise<void>;
  constructor?: LovelaceCardConstructor;
}

declare global {
  interface HASSDomEvents {
    "ll-rebuild": Record<string, unknown>;
    "ll-upgrade": Record<string, unknown>;
    "ll-edit-card": { sectionIndex: number; cardIndex: number };
    "ll-duplicate-card": { sectionIndex: number; cardIndex: number };
    "ll-copy-card": { sectionIndex: number; cardIndex: number };
    "ll-delete-card": { sectionIndex: number; cardIndex: number; silent?: boolean };
    "ll-edit-section": { sectionIndex: number };
    "ll-duplicate-section": { sectionIndex: number };
    "ll-delete-section": { sectionIndex: number };
    "item-moved": { oldIndex: number; newIndex: number };
    "item-added": { index: number; data: unknown; item: HTMLElement };
    "item-removed": { index: number };
    "drag-start": undefined;
    "drag-end": undefined;
    "hass-toggle-menu": undefined | { open?: boolean };
    "hass-drawer-closed": undefined;
    "hass-dock-sidebar": { dock: "docked" | "auto" };
    "picture-element-position": { left: number; top: number };
    "picture-element-select": { index: number };
    "picture-elements-changed": { config: LovelaceCardConfig };
  }
}
