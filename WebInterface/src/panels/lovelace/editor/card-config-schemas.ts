import type { ConfigFieldSchema } from "../../../types";
import { CHART2D_DEFAULTS } from "../../../common/chart/chart-2d-entity";
import { CHART3D_DEFAULTS } from "../../../common/chart/chart-3d-entity";
import { CONTENT_LAYOUT_FIELD, entityCardSchema } from "./card-common-schemas";

const CHART2D_APPEARANCE_FIELDS: ConfigFieldSchema[] = [
  { name: "bg_color", label: "背景色", type: "color", optional: true },
];

const CHART2D_AXIS_FIELDS: ConfigFieldSchema[] = [
  { name: "x_label", label: "X 轴标签", type: "text", optional: true, placeholder: CHART2D_DEFAULTS.x_label },
  { name: "y_label", label: "Y 轴标签", type: "text", optional: true, placeholder: CHART2D_DEFAULTS.y_label },
  { name: "show_axes", label: "显示坐标轴", type: "boolean" },
  { name: "show_grid", label: "显示网格线", type: "boolean" },
];

const CHART2D_DATA_FIELDS: ConfigFieldSchema[] = [
  {
    name: "max_points",
    label: "最大点数",
    type: "number",
    helper: "超出后丢弃最旧的数据点",
  },
];

const CHART3D_APPEARANCE_FIELDS: ConfigFieldSchema[] = [
  { name: "bg_color", label: "背景色", type: "color", optional: true },
];

const CHART3D_AXIS_FIELDS: ConfigFieldSchema[] = [
  { name: "x_label", label: "X 轴标签", type: "text", optional: true, placeholder: CHART3D_DEFAULTS.x_label },
  { name: "y_label", label: "Y 轴标签", type: "text", optional: true, placeholder: CHART3D_DEFAULTS.y_label },
  { name: "z_label", label: "Z 轴标签", type: "text", optional: true, placeholder: CHART3D_DEFAULTS.z_label },
  { name: "show_axes", label: "显示坐标轴", type: "boolean" },
  { name: "auto_rotate", label: "自动旋转", type: "boolean" },
];

const CHART3D_DATA_FIELDS: ConfigFieldSchema[] = [
  {
    name: "max_points",
    label: "最大点数",
    type: "number",
    helper: "超出后丢弃最旧的数据点",
  },
];

/** Card config schemas aligned with Home Assistant card editors */
export const CARD_CONFIG_SCHEMAS: Record<string, ConfigFieldSchema[]> = {
  heading: [
    {
      name: "heading_style",
      label: "样式",
      type: "select",
      options: [
        { value: "title", label: "Title" },
        { value: "subtitle", label: "副标题" },
      ],
    },
    { name: "heading", label: "标题文字", type: "text", placeholder: "客厅" },
    { name: "icon", label: "图标", type: "icon", placeholder: "mdi:sofa", optional: true },
    { name: "badges", label: "徽章", type: "badges" },
  ],

  tile: [
    ...entityCardSchema([
      { name: "hide_state", label: "隐藏状态", type: "boolean" },
      CONTENT_LAYOUT_FIELD,
    ]),
  ],

  sensor: [
    ...entityCardSchema([
      { name: "hide_state", label: "隐藏状态", type: "boolean" },
      CONTENT_LAYOUT_FIELD,
    ]),
    { name: "unit", label: "单位", type: "text", optional: true, placeholder: "°C" },
  ],

  label: [
    ...entityCardSchema([
      { name: "hide_state", label: "隐藏状态", type: "boolean" },
      CONTENT_LAYOUT_FIELD,
    ]),
  ],

  switch: [...entityCardSchema([CONTENT_LAYOUT_FIELD])],

  trigger: [...entityCardSchema([CONTENT_LAYOUT_FIELD])],

  slider: [
    ...entityCardSchema([CONTENT_LAYOUT_FIELD]),
    {
      name: "range",
      label: "范围",
      type: "grid",
      schema: [
        { name: "min", label: "最小值", type: "number" },
        { name: "max", label: "最大值", type: "number" },
        { name: "step", label: "步进", type: "number" },
      ],
    },
  ],

  climate: [
    ...entityCardSchema([CONTENT_LAYOUT_FIELD]),
    {
      name: "range",
      label: "范围",
      type: "grid",
      schema: [
        { name: "min", label: "最小值", type: "number" },
        { name: "max", label: "最大值", type: "number" },
        { name: "step", label: "步进", type: "number" },
      ],
    },
    { name: "unit", label: "单位", type: "text", optional: true, placeholder: "°C" },
  ],

  gain: [
    ...entityCardSchema([CONTENT_LAYOUT_FIELD]),
    {
      name: "range",
      label: "范围 (dB)",
      type: "grid",
      schema: [
        { name: "min", label: "最小值", type: "number" },
        { name: "max", label: "最大值", type: "number" },
        { name: "step", label: "步进", type: "number" },
      ],
    },
  ],

  rgba: [...entityCardSchema([CONTENT_LAYOUT_FIELD])],

  hsv: [...entityCardSchema([CONTENT_LAYOUT_FIELD])],

  /** @deprecated use rgba */
  color: [...entityCardSchema([CONTENT_LAYOUT_FIELD])],

  "multi-fader": [
    ...entityCardSchema([
      CONTENT_LAYOUT_FIELD,
      { name: "hide_state", label: "隐藏状态", type: "boolean" },
    ]),
    {
      name: "count",
      label: "维度数量",
      type: "number",
      helper: "推杆个数（1–16），输出对应长度的数组",
    },
    {
      name: "labels",
      label: "通道标签",
      type: "text",
      optional: true,
      placeholder: "0,1,2,3",
      helper: "用逗号分隔，数量不足时从 0 自动补序号",
    },
    {
      name: "orientation",
      label: "推杆方向",
      type: "select",
      options: [
        { value: "vertical", label: "竖直" },
        { value: "horizontal", label: "水平" },
      ],
    },
    {
      name: "range",
      label: "范围",
      type: "grid",
      schema: [
        { name: "min", label: "最小值", type: "number" },
        { name: "max", label: "最大值", type: "number" },
        { name: "step", label: "步进", type: "number" },
      ],
    },
  ],

  "xy-pad": [
    {
      name: "entity",
      label: "动作（向量）",
      type: "entity",
      optional: true,
      helper: "写入 [x, y] 数组；若同时填写 X/Y 动作则优先使用分动作模式",
    },
    {
      name: "axes",
      label: "分动作（可选）",
      type: "grid",
      schema: [
        {
          name: "entity_x",
          label: "X 动作",
          type: "entity",
          optional: true,
        },
        {
          name: "entity_y",
          label: "Y 动作",
          type: "entity",
          optional: true,
        },
      ],
    },
    {
      name: "content",
      label: "内容",
      type: "section",
      schema: [
        {
          name: "name",
          label: "名称",
          type: "text",
          optional: true,
          helper: "留空则使用地址默认名称",
        },
        {
          name: "",
          label: "",
          type: "grid",
          gridVariant: "appearance",
          schema: [
            { name: "icon", label: "图标", type: "icon", optional: true, compact: true },
            { name: "color", label: "颜色", type: "color", compact: true },
          ],
        },
        CONTENT_LAYOUT_FIELD,
        { name: "show_grid", label: "显示十字线", type: "boolean" },
        { name: "invert_y", label: "Y 轴向上为正", type: "boolean", helper: "开启后触控板顶部对应 y_max" },
        { name: "snap_center", label: "松手回中", type: "boolean" },
      ],
    },
    {
      name: "range",
      label: "范围",
      type: "grid",
      schema: [
        { name: "x_min", label: "X 最小", type: "number" },
        { name: "x_max", label: "X 最大", type: "number" },
        { name: "y_min", label: "Y 最小", type: "number" },
        { name: "y_max", label: "Y 最大", type: "number" },
        { name: "step", label: "步进", type: "number" },
      ],
    },
  ],

  cover: [
    ...entityCardSchema([CONTENT_LAYOUT_FIELD]),
    {
      name: "position_count",
      label: "档位数量",
      type: "number",
      helper: "默认生成 0、1、2… 档位；留空自定义 positions 时优先使用 positions",
    },
  ],

  media: [
    ...entityCardSchema([CONTENT_LAYOUT_FIELD]),
    {
      name: "play_value",
      label: "播放指令",
      type: "text",
      optional: true,
      placeholder: "true",
      helper: "写入地址的值，支持 true / false / 数字 / 字符串",
    },
    {
      name: "stop_value",
      label: "停止指令",
      type: "text",
      optional: true,
      placeholder: "false",
    },
  ],

  clock: [
    { name: "title", label: "标题", type: "text", optional: true },
    {
      name: "clock_style",
      label: "样式",
      type: "select",
      options: [
        { value: "digital", label: "数字" },
        { value: "analog", label: "模拟" },
      ],
    },
    {
      name: "clock_size",
      label: "尺寸",
      type: "select",
      options: [
        { value: "small", label: "小" },
        { value: "medium", label: "中" },
        { value: "large", label: "大" },
      ],
    },
    {
      name: "time_format",
      label: "时间格式",
      type: "select",
      options: [
        { value: "auto", label: "跟随系统" },
        { value: "24", label: "24 小时" },
        { value: "12", label: "12 小时" },
      ],
      optional: true,
    },
    { name: "show_seconds", label: "显示秒", type: "boolean" },
    { name: "no_background", label: "无背景", type: "boolean" },
    {
      name: "time_zone",
      label: "时区",
      type: "text",
      optional: true,
      placeholder: "Asia/Shanghai",
      helper: "留空则使用浏览器时区",
    },
    {
      name: "analog_border",
      label: "表盘边框",
      type: "boolean",
      visible: { field: "clock_style", operator: "eq", value: "analog" },
    },
    {
      name: "analog_face_style",
      label: "表盘数字",
      type: "select",
      options: [
        { value: "markers", label: "刻度" },
        { value: "numbers", label: "阿拉伯数字" },
        { value: "roman", label: "罗马数字" },
      ],
      visible: { field: "clock_style", operator: "eq", value: "analog" },
    },
    {
      name: "analog_ticks",
      label: "刻度",
      type: "select",
      options: [
        { value: "hour", label: "小时" },
        { value: "quarter", label: "一刻钟" },
        { value: "minute", label: "分钟" },
        { value: "none", label: "无" },
      ],
      visible: { field: "clock_style", operator: "eq", value: "analog" },
    },
  ],

  markdown: [
    {
      name: "style",
      label: "样式",
      type: "select",
      options: [
        { value: "card", label: "卡片" },
        { value: "text-only", label: "纯文本" },
      ],
    },
    {
      name: "title",
      label: "标题",
      type: "text",
      optional: true,
      visible: { field: "style", operator: "not_eq", value: "text-only" },
    },
    {
      name: "content",
      label: "内容",
      type: "textarea",
      helper: "支持 Markdown 语法",
    },
  ],

  link: [
    { name: "name", label: "名称", type: "text", placeholder: "文档" },
    { name: "url", label: "链接地址", type: "text", placeholder: "http://127.0.0.1:8992/" },
    { name: "icon", label: "图标", type: "icon", optional: true, placeholder: "mdi:link-variant" },
    { name: "new_tab", label: "新标签页打开", type: "boolean" },
  ],

  "picture-elements": [
    { name: "title", label: "标题", type: "text", optional: true, placeholder: "户型图" },
    { name: "image", label: "底图", type: "image" },
    { name: "elements", label: "元素", type: "picture_elements" },
  ],

  line3d: [
    { name: "entity", label: "动作", type: "entity" },
    { name: "name", label: "名称", type: "text", optional: true },
    ...CHART3D_DATA_FIELDS,
    { name: "line_width", label: "线宽", type: "number" },
    { name: "line_color", label: "线条颜色", type: "color", optional: true },
    ...CHART3D_APPEARANCE_FIELDS,
    ...CHART3D_AXIS_FIELDS,
  ],

  scatter3d: [
    { name: "entity", label: "动作", type: "entity" },
    { name: "name", label: "名称", type: "text", optional: true },
    ...CHART3D_DATA_FIELDS,
    { name: "symbol_size", label: "点大小", type: "number" },
    { name: "point_color", label: "点颜色", type: "color", optional: true },
    ...CHART3D_APPEARANCE_FIELDS,
    ...CHART3D_AXIS_FIELDS,
  ],

  line2d: [
    { name: "entity", label: "动作", type: "entity" },
    { name: "name", label: "名称", type: "text", optional: true },
    ...CHART2D_DATA_FIELDS,
    { name: "line_width", label: "线宽", type: "number" },
    { name: "line_color", label: "线条颜色", type: "color", optional: true },
    ...CHART2D_APPEARANCE_FIELDS,
    ...CHART2D_AXIS_FIELDS,
  ],

  scatter2d: [
    { name: "entity", label: "动作", type: "entity" },
    { name: "name", label: "名称", type: "text", optional: true },
    ...CHART2D_DATA_FIELDS,
    { name: "symbol_size", label: "点大小", type: "number" },
    { name: "point_color", label: "点颜色", type: "color", optional: true },
    ...CHART2D_APPEARANCE_FIELDS,
    ...CHART2D_AXIS_FIELDS,
  ],

  bar: [
    { name: "name", label: "名称", type: "text", optional: true },
    { name: "entities", label: "柱", type: "bar_entities" },
    { name: "bar_color", label: "默认柱颜色", type: "color", optional: true, helper: "未单独设色的柱使用此颜色" },
    ...CHART2D_APPEARANCE_FIELDS,
    ...CHART2D_AXIS_FIELDS,
  ],

  status: [
    { name: "name", label: "名称", type: "text", optional: true, placeholder: "Flow 连接" },
    {
      name: "",
      label: "",
      type: "grid",
      gridVariant: "appearance",
      schema: [
        { name: "icon", label: "图标", type: "icon", optional: true, compact: true },
        { name: "color", label: "正常色", type: "color", compact: true },
      ],
    },
    CONTENT_LAYOUT_FIELD,
    {
      name: "list_columns",
      label: "状态列数",
      type: "select",
      options: [
        { value: "1", label: "单列" },
        { value: "2", label: "双列" },
      ],
    },
    { name: "entities", label: "状态项", type: "status_entities" },
  ],
};

export function getCardConfigSchema(type: string): ConfigFieldSchema[] {
  const resolved =
    type === "button" ? "trigger" : type === "color" ? "rgba" : type;
  return CARD_CONFIG_SCHEMAS[resolved] ?? [
    { name: "entity", label: "动作", type: "entity", optional: true },
    { name: "name", label: "名称", type: "text", optional: true },
  ];
}

/** Sync grid rows/columns when content layout changes (HA tile/slider behavior). */
export function syncLayoutGridOptions(config: Record<string, unknown>): void {
  const type = config.type;
  const vertical = Boolean(config.vertical);
  const existing = { ...((config.grid_options ?? {}) as Record<string, unknown>) };

  if (type === "tile" || type === "trigger" || type === "sensor" || type === "label") {
    const rows = vertical ? 2 : 1;
    config.grid_options = {
      ...existing,
      columns: existing.columns ?? 6,
      rows,
      min_rows: rows,
      min_columns: vertical ? 3 : 6,
    };
    return;
  }

  if (type === "multi-fader") {
    const count = Math.max(1, Math.min(16, Math.round(Number(config.count) || 4)));
    const orientation = config.orientation === "horizontal" ? "horizontal" : "vertical";
    const rows =
      orientation === "vertical" ? 4 : Math.max(2, Math.min(8, 1 + Math.ceil(count / 2)));
    config.grid_options = {
      ...existing,
      columns: existing.columns ?? (orientation === "vertical" ? Math.min(12, Math.max(6, count * 2)) : 6),
      rows,
      min_rows: orientation === "vertical" ? 3 : rows,
      min_columns: vertical ? 3 : 6,
    };
    return;
  }

  if (type === "xy-pad" || type === "rgba" || type === "hsv" || type === "color") {
    const rows = type === "xy-pad" ? (vertical ? 4 : 3) : 5;
    config.grid_options = {
      ...existing,
      columns: existing.columns ?? 6,
      rows,
      min_rows: rows,
      min_columns: vertical ? 3 : 6,
    };
    return;
  }

  if (type === "slider" || type === "switch" || type === "climate" || type === "cover" || type === "gain" || type === "media") {
    const rows = vertical ? 3 : 2;
    config.grid_options = {
      ...existing,
      columns: existing.columns ?? 6,
      rows,
      min_rows: rows,
      min_columns: vertical ? 3 : 6,
    };
    return;
  }

  if (type === "status") {
    config.grid_options = {
      ...existing,
      columns: existing.columns ?? (vertical ? 3 : 6),
      rows: "auto",
      min_rows: 2,
      min_columns: vertical ? 3 : 6,
    };
  }
}

/** Convert editor values to persisted card config */
export function normalizeCardConfig(
  config: Record<string, unknown>,
  options?: { layoutChanged?: boolean },
): Record<string, unknown> {
  const next = { ...config };

  if (next.type === "button") {
    next.type = "trigger";
  }

  if (next.type === "color") {
    next.type = "rgba";
  }

  if (next.content_layout !== undefined) {
    next.vertical = next.content_layout === "vertical";
    delete next.content_layout;
  }

  if (
    options?.layoutChanged &&
    (next.type === "tile" ||
      next.type === "trigger" ||
      next.type === "sensor" ||
      next.type === "label" ||
      next.type === "slider" ||
      next.type === "switch" ||
      next.type === "climate" ||
      next.type === "cover" ||
      next.type === "gain" ||
      next.type === "media" ||
      next.type === "rgba" ||
      next.type === "hsv" ||
      next.type === "color" ||
      next.type === "xy-pad" ||
      next.type === "multi-fader" ||
      next.type === "status")
  ) {
    syncLayoutGridOptions(next);
  }

  if (next.style !== undefined) {
    if (next.style === "text-only") {
      next.text_only = true;
    } else {
      delete next.text_only;
    }
    delete next.style;
  }

  if (next.time_format === "auto") {
    delete next.time_format;
  }

  if (!next.color) {
    delete next.color;
  }

  const stripKeys = next.type === "markdown" ? ["range", ""] : ["content", "range", ""];
  for (const key of stripKeys) {
    if (key in next) delete next[key];
  }

  if (next.type === "status") {
    const cols = Number(next.list_columns);
    if (cols === 2) {
      next.list_columns = 2;
    } else {
      delete next.list_columns;
    }
  }

  return next;
}

/** Convert persisted config to editor form values */
export function denormalizeCardConfig(config: Record<string, unknown>): Record<string, unknown> {
  const next = { ...config };

  if ("vertical" in next) {
    next.content_layout = next.vertical ? "vertical" : "horizontal";
  } else if (
    next.type === "tile" ||
    next.type === "trigger" ||
    next.type === "slider" ||
    next.type === "switch" ||
    next.type === "climate" ||
    next.type === "cover" ||
    next.type === "gain" ||
    next.type === "media" ||
    next.type === "status"
  ) {
    next.content_layout = "horizontal";
  }

  if (next.text_only) {
    next.style = "text-only";
  } else if (next.type === "markdown") {
    next.style = "card";
  }

  if (next.type === "clock" && !next.time_format) {
    next.time_format = "auto";
  }

  if (next.type === "climate") {
    if (next.min === undefined) next.min = 0;
    if (next.max === undefined) next.max = 100;
    if (next.step === undefined) next.step = 1;
  }

  if (next.type === "gain") {
    if (next.min === undefined) next.min = -60;
    if (next.max === undefined) next.max = 12;
    if (next.step === undefined) next.step = 1;
  }

  if (next.type === "cover" && next.position_count === undefined) {
    next.position_count = 4;
  }

  if (next.type === "clock") {
    if (!next.clock_style) next.clock_style = "digital";
    if (!next.clock_size) next.clock_size = "small";
  }

  if (next.type === "clock" && next.analog_border === undefined && next.clock_style === "analog") {
    next.analog_border = true;
  }

  if (next.type === "clock" && !next.analog_face_style && next.clock_style === "analog") {
    next.analog_face_style = "markers";
  }

  if (next.type === "heading") {
    if (!next.heading_style) next.heading_style = "title";
    if (!next.heading) next.heading = "新建部件";
    if (!next.badges) next.badges = [];
  }

  if (!next.elements && next.type === "picture-elements") {
    next.elements = [];
  }

  if (next.type === "line3d") {
    if (next.max_points === undefined) next.max_points = CHART3D_DEFAULTS.max_points;
    if (next.line_width === undefined) next.line_width = CHART3D_DEFAULTS.line_width;
    if (next.show_axes === undefined) next.show_axes = CHART3D_DEFAULTS.show_axes;
    if (next.auto_rotate === undefined) next.auto_rotate = CHART3D_DEFAULTS.auto_rotate;
  }

  if (next.type === "scatter3d") {
    if (next.max_points === undefined) next.max_points = CHART3D_DEFAULTS.max_points;
    if (next.symbol_size === undefined) next.symbol_size = CHART3D_DEFAULTS.symbol_size;
    if (next.show_axes === undefined) next.show_axes = CHART3D_DEFAULTS.show_axes;
    if (next.auto_rotate === undefined) next.auto_rotate = CHART3D_DEFAULTS.auto_rotate;
  }

  if (next.type === "line2d") {
    if (next.max_points === undefined) next.max_points = CHART2D_DEFAULTS.max_points;
    if (next.line_width === undefined) next.line_width = CHART2D_DEFAULTS.line_width;
    if (next.show_axes === undefined) next.show_axes = CHART2D_DEFAULTS.show_axes;
    if (next.show_grid === undefined) next.show_grid = CHART2D_DEFAULTS.show_grid;
  }

  if (next.type === "scatter2d") {
    if (next.max_points === undefined) next.max_points = CHART2D_DEFAULTS.max_points;
    if (next.symbol_size === undefined) next.symbol_size = CHART2D_DEFAULTS.symbol_size;
    if (next.show_axes === undefined) next.show_axes = CHART2D_DEFAULTS.show_axes;
    if (next.show_grid === undefined) next.show_grid = CHART2D_DEFAULTS.show_grid;
  }

  if (next.type === "bar") {
    if (!next.entities) next.entities = [];
    if (next.show_axes === undefined) next.show_axes = CHART2D_DEFAULTS.show_axes;
    if (next.show_grid === undefined) next.show_grid = CHART2D_DEFAULTS.show_grid;
  }

  if (next.type === "status") {
    if (!next.entities) next.entities = [];
    if (!next.icon) next.icon = "mdi:lan-connect";
    next.list_columns = Number(next.list_columns) === 2 ? "2" : "1";
  }

  if (next.type === "link") {
    if (next.new_tab === undefined) next.new_tab = true;
    if (!next.url) next.url = "http://127.0.0.1:8992/";
  }

  return next;
}
