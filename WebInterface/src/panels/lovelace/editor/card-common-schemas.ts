import type { ConfigFieldSchema } from "../../../types";

/** HA tile content-layout preview SVGs (light theme). */
const TILE_LAYOUT_HORIZONTAL_SVG =
  "data:image/svg+xml," +
  encodeURIComponent(
    `<svg xmlns="http://www.w3.org/2000/svg" width="96" height="56" viewBox="0 0 96 56" fill="none">
      <rect x="10" y="12" width="76" height="32" rx="6" fill="#fff" stroke="#e0e0e0"/>
      <circle cx="28" cy="28" r="8" fill="#cfd8dc"/>
      <rect x="44" y="22" width="30" height="5" rx="2.5" fill="#cfd8dc"/>
      <rect x="44" y="31" width="20" height="5" rx="2.5" fill="#cfd8dc"/>
    </svg>`,
  );

const TILE_LAYOUT_VERTICAL_SVG =
  "data:image/svg+xml," +
  encodeURIComponent(
    `<svg xmlns="http://www.w3.org/2000/svg" width="96" height="56" viewBox="0 0 96 56" fill="none">
      <rect x="22" y="6" width="52" height="44" rx="6" fill="#fff" stroke="#e0e0e0"/>
      <circle cx="48" cy="20" r="7" fill="#cfd8dc"/>
      <rect x="34" y="32" width="28" height="4" rx="2" fill="#cfd8dc"/>
      <rect x="38" y="39" width="20" height="4" rx="2" fill="#cfd8dc"/>
    </svg>`,
  );

/** Icon + color row (HA tile card content appearance) */
export const APPEARANCE_GRID: ConfigFieldSchema = {
  name: "",
  label: "",
  type: "grid",
  gridVariant: "appearance",
  schema: [
    { name: "icon", label: "图标", type: "icon", optional: true, compact: true },
    { name: "color", label: "颜色", type: "color", compact: true },
  ],
};

/** HA-style content layout box selector (horizontal / vertical). */
export const CONTENT_LAYOUT_FIELD: ConfigFieldSchema = {
  name: "content_layout",
  label: "内容布局",
  type: "select_box",
  options: [
    { value: "horizontal", label: "水平", image: TILE_LAYOUT_HORIZONTAL_SVG },
    { value: "vertical", label: "垂直", image: TILE_LAYOUT_VERTICAL_SVG },
  ],
};

export const CARD_CONTENT_SECTION: ConfigFieldSchema = {
  name: "content",
  label: "内容",
  type: "section",
  schema: [
    {
      name: "name",
      label: "名称",
      type: "text",
      optional: true,
      helper: "留空则使用动作默认名称",
    },
    APPEARANCE_GRID,
  ],
};

export const ENTITY_ADDRESS_FIELD: ConfigFieldSchema = {
  name: "entity",
  label: "动作",
  type: "entity",
};

/** Base schema shared by entity-based cards (tile, sensor, switch, trigger, slider) */
export function entityCardSchema(extraContentFields: ConfigFieldSchema[] = []): ConfigFieldSchema[] {
  const contentSchema = [
    ...(CARD_CONTENT_SECTION.schema ?? []),
    ...extraContentFields,
  ];
  return [
    ENTITY_ADDRESS_FIELD,
    {
      ...CARD_CONTENT_SECTION,
      schema: contentSchema,
    },
  ];
}
