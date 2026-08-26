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

export interface LovelaceHeadingBadgeConfig {
  type: "entity" | "button" | string;
  entity?: string;
  name?: string;
  icon?: string;
  color?: string;
  /** entity / button: show icon (state only, default true) */
  show_icon?: boolean;
  /** show text: live state for entity, label for button (default true) */
  show_name?: boolean;
  /** @deprecated use show_name */
  show_state?: boolean;
  /** button: label text */
  text?: string;
  /** button: value written to entity on tap (default true) */
  value?: FlowValue;
}

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

/** Overlay on a picture-elements card (HA state-badge / button). */
export interface LovelacePictureElementConfig {
  type: "state-badge" | "button" | string;
  entity?: string;
  name?: string;
  icon?: string;
  color?: string;
  unit?: string;
  text?: string;
  show_icon?: boolean;
  show_name?: boolean;
  show_state?: boolean;
  value?: FlowValue;
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
    | "picture_elements";
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
    "picture-elements-changed": { config: LovelaceCardConfig };
  }
}
