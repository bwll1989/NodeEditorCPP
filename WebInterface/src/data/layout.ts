import type { LovelaceConfig } from "../types";
import { migrateConfig } from "./section-config";

const WELCOME_MARKDOWN = `欢迎使用 **Flow Dashboard**。

布局采用 Section + Card 结构，仿照 [Home Assistant Sections](https://www.home-assistant.io/dashboards/sections/)。

- 支持 **粗体** / *斜体*
- 支持列表与链接
- 可在编辑器中切换卡片 / 纯文本样式`;

/** 与产品演示一致的 Home 默认布局（空白页 / 无布局时回退） */
function createDefaultHomeLayout(): LovelaceConfig {
  return migrateConfig({
    title: "Flow",
    views: [
      {
        title: "Home",
        path: "home",
        icon: "mdi:home",
        max_columns: 4,
        sections: [
          {
            type: "grid",
            column_span: 4,
            cards: [
              {
                type: "heading",
                heading: "Welcome",
                icon: "mdi:hand-wave",
                badges: [
                  {
                    type: "entity",
                    entity: "/demo/temperature",
                    icon: "mdi:thermometer",
                  },
                  {
                    type: "entity",
                    entity: "/demo/humidity",
                    icon: "mdi:water-percent",
                  },
                ],
              },
              {
                type: "markdown",
                content: WELCOME_MARKDOWN,
              },
            ],
          },
          {
            type: "grid",
            column_span: 1,
            cards: [
              {
                type: "clock",
                clock_style: "analog",
                clock_size: "small",
                time_format: "24",
                grid_options: { columns: 6, rows: 2, min_columns: 3, min_rows: 2 },
              },
              {
                type: "media",
                entity: "/demo/media",
                name: "媒体播放器",
                icon: "mdi:cast-audio",
              },
              {
                type: "slider",
                entity: "/demo/spotlights",
                name: "Spotlights",
                icon: "mdi:spotlight-beam",
              },
              {
                type: "rgba",
                entity: "/demo/rgba",
                name: "RGBA",
                icon: "mdi:palette",
                grid_options: { columns: 6, rows: 5, min_columns: 6, min_rows: 5 },
              },
              {
                type: "hsv",
                entity: "/demo/hsv",
                name: "HSV",
                icon: "mdi:palette-swatch",
                grid_options: { columns: 6, rows: 5, min_columns: 6, min_rows: 5 },
              },
              {
                type: "multi-fader",
                entity: "/demo/faders",
                name: "多维推杆",
                icon: "mdi:tune-vertical-variant",
                count: 4,
                min: 0,
                max: 1,
                step: 0.01,
                labels: "0,1,2,3",
                orientation: "vertical",
              },
              {
                type: "xy-pad",
                entity: "/demo/xy",
                name: "XY Pad",
                icon: "mdi:axis-arrow",
              },
              {
                type: "climate",
                entity: "/demo/climate",
                name: "Upstairs",
                icon: "mdi:home-thermometer",
                min: 0,
                max: 100,
                step: 1,
                unit: "°C",
              },
              {
                type: "sensor",
                entity: "/demo/temperature",
                name: "Temperature",
                icon: "mdi:thermometer",
                unit: "°C",
              },
              {
                type: "label",
                entity: "/demo/string",
                name: "当前素材",
                icon: "mdi:filmstrip",
              },
              {
                type: "status",
                name: "Flow 连接",
                icon: "mdi:lan-connect",
                color: "green",
                entities: [
                  { entity: "/demo/status_ws", name: "connected", icon: "mdi:lan-connect" },
                  { entity: "/demo/status_tcp", name: "tcp连接", icon: "mdi:lan-connect" },
                  { entity: "/demo/status_udp", name: "connected", icon: "mdi:lan-connect" },
                  { entity: "/demo/status_osc", name: "connected", icon: "mdi:lan-connect" },
                ],
                grid_options: { columns: 6, rows: "auto", min_columns: 3 },
              },
            ],
          },
          {
            type: "grid",
            column_span: 1,
            cards: [
              {
                type: "switch",
                entity: "/demo/floor_lamp",
                name: "Flood light",
                icon: "mdi:lightbulb",
              },
              {
                type: "cover",
                entity: "/demo/kitchen_shutter",
                name: "Kitchen shutter",
                icon: "mdi:window-shutter",
                position_count: 4,
              },
              {
                type: "trigger",
                entity: "/demo/trigger",
                name: "Trigger",
                icon: "mdi:gesture-tap-button",
              },
            ],
          },
          {
            type: "grid",
            column_span: 1,
            cards: [
              {
                type: "gain",
                entity: "/demo/gain",
                name: "输入增益",
                icon: "mdi:volume-high",
                min: -60,
                max: 12,
                step: 1,
              },
            ],
          },
        ],
      },
      {
        title: "设备",
        path: "devices",
        icon: "mdi:devices",
        max_columns: 4,
        sections: [
          {
            type: "grid",
            column_span: 2,
            cards: [
              {
                type: "heading",
                heading: "连接状态",
                icon: "mdi:lan-connect",
              },
              {
                type: "status",
                name: "Flow 连接",
                icon: "mdi:lan-connect",
                color: "green",
                list_columns: 2,
                entities: [
                  { entity: "/demo/status_ws", name: "connected", icon: "mdi:lan-connect" },
                  { entity: "/demo/status_tcp", name: "tcp连接", icon: "mdi:lan-connect" },
                  { entity: "/demo/status_udp", name: "connected", icon: "mdi:lan-connect" },
                  { entity: "/demo/status_osc", name: "connected", icon: "mdi:lan-connect" },
                ],
              },
              {
                type: "tile",
                entity: "/demo/lamp",
                name: "台灯",
                icon: "mdi:desk-lamp",
              },
              {
                type: "switch",
                entity: "/demo/switch",
                name: "开关",
                icon: "mdi:toggle-switch",
              },
            ],
          },
        ],
      },
    ],
  });
}

const DEMO_CONFIG = createDefaultHomeLayout();

function isLayoutPayloadEmpty(data: {
  version?: number;
  views?: LovelaceConfig["views"];
  items?: unknown[];
}): boolean {
  if (data.version === 2) {
    if (!Array.isArray(data.views) || data.views.length === 0) {
      return true;
    }
    return data.views.every((view) => {
      const sections = view.sections ?? [];
      if (sections.length === 0) return true;
      return sections.every((section) => (section.cards?.length ?? 0) === 0);
    });
  }
  if (Array.isArray(data.items)) {
    return data.items.length === 0;
  }
  return true;
}

export async function fetchLayout(): Promise<LovelaceConfig> {
  try {
    const res = await fetch("/api/layout/load");
    if (!res.ok) return DEMO_CONFIG;
    const data = (await res.json()) as {
      version?: number;
      views?: LovelaceConfig["views"];
      items?: unknown[];
    };
    if (isLayoutPayloadEmpty(data)) {
      return DEMO_CONFIG;
    }
    if (data.version === 2 && Array.isArray(data.views)) {
      return migrateConfig({ title: "Flow", views: data.views });
    }
    return DEMO_CONFIG;
  } catch {
    return DEMO_CONFIG;
  }
}

export async function saveLayout(config: LovelaceConfig): Promise<void> {
  await fetch("/api/layout/save", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ version: 2, ...config }),
  });
}

export function collectEntities(config: LovelaceConfig): string[] {
  const set = new Set<string>();
  config.views.forEach((view) => {
    view.sections.forEach((section) => {
      section.cards.forEach((card) => {
        if (card.entity) set.add(card.entity);
        const entityX = card.entity_x;
        if (typeof entityX === "string" && entityX) set.add(entityX);
        const entityY = card.entity_y;
        if (typeof entityY === "string" && entityY) set.add(entityY);
        const badges = card.badges as { entity?: string }[] | undefined;
        badges?.forEach((b) => {
          if (b.entity) set.add(b.entity);
        });
        const elements = card.elements as { entity?: string }[] | undefined;
        elements?.forEach((el) => {
          if (el.entity) set.add(el.entity);
        });
        const barEntities = card.entities as { entity?: string }[] | undefined;
        barEntities?.forEach((item) => {
          if (item.entity) set.add(item.entity);
        });
      });
    });
  });
  return [...set];
}

export { DEMO_CONFIG, createDefaultHomeLayout };
