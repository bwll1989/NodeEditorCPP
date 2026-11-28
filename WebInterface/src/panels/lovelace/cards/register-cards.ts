import { registerCard } from "../../../registry/card-registry";
import { t } from "../../../common/i18n/i18n";
import type { CardRegistryEntry, LovelaceCardConstructor } from "../../../types";
import { HuiHeadingCard } from "./hui-heading-card";
import { HuiErrorCard } from "./hui-error-card";
import { HuiMarkdownCard } from "./hui-markdown-card";
import { HuiSensorCard } from "./hui-sensor-card";
import { HuiLabelCard } from "./hui-label-card";
import { HuiSliderCard } from "./hui-slider-card";
import { HuiSwitchCard } from "./hui-switch-card";
import { HuiTileCard } from "./hui-tile-card";
import { HuiClockCard } from "./hui-clock-card";
import { HuiClimateCard } from "./hui-climate-card";
import { HuiCoverCard } from "./hui-cover-card";
import { HuiTriggerCard } from "./hui-trigger-card";
import { HuiPictureElementsCard } from "./hui-picture-elements-card";
import { HuiLinkCard } from "./hui-link-card";
import { HuiGainCard } from "./hui-gain-card";
import { HuiRgbaCard, HuiColorCard } from "./hui-rgba-card";
import { HuiHsvCard } from "./hui-hsv-card";
import { HuiMultiFaderCard } from "./hui-multi-fader-card";
import { HuiXyPadCard } from "./hui-xy-pad-card";
import { HuiMediaCard } from "./hui-media-card";
import { HuiMicCard } from "./hui-mic-card";
import { HuiStatusCard } from "./hui-status-card";
import { HuiPathBadgeCard } from "./hui-path-badge-card";

function registerLazyChartCard(
  type: string,
  name: string,
  description: string,
  icon: string,
  loader: () => Promise<{ default?: LovelaceCardConstructor } & Record<string, LovelaceCardConstructor>>,
  exportName: string,
): void {
  const entry = {
    type,
    name,
    description,
    icon,
    category: "sensor" as const,
    lazy: true,
    loader: async () => {
      const mod = await loader();
      entry.constructor = mod[exportName];
    },
  } as CardRegistryEntry;
  registerCard(entry);
}

export function registerBuiltinCards(): void {
  registerCard({
    type: "heading",
    name: t("标题"),
    description: t("分区标题行（含状态徽章与控制徽章）"),
    icon: "mdi:format-title",
    category: "container",
    constructor: HuiHeadingCard,
  });
  registerCard({
    type: "tile",
    name: t("磁贴"),
    description: t("开关切换磁贴"),
    icon: "mdi:view-grid",
    category: "tile",
    constructor: HuiTileCard,
  });
  registerCard({
    type: "sensor",
    name: t("数值"),
    description: t("显示地址数值"),
    icon: "mdi:gauge",
    category: "sensor",
    constructor: HuiSensorCard,
  });
  registerCard({
    type: "label",
    name: t("标签"),
    description: t("只读显示字符串（设备名、素材名、错误信息等）"),
    icon: "mdi:label-outline",
    category: "sensor",
    constructor: HuiLabelCard,
  });
  registerCard({
    type: "status",
    name: t("状态"),
    description: t("多地址状态列表（汇总正常/异常）"),
    icon: "mdi:lan-connect",
    category: "sensor",
    constructor: HuiStatusCard,
  });
  registerCard({
    type: "clock",
    name: t("时钟"),
    description: t("数字/模拟时钟"),
    icon: "mdi:clock-outline",
    category: "container",
    constructor: HuiClockCard,
  });
  registerCard({
    type: "climate",
    name: t("步进"),
    description: t("步进磁贴（目标值 +/-）"),
    icon: "mdi:home-thermometer",
    category: "control",
    constructor: HuiClimateCard,
  });
  registerCard({
    type: "cover",
    name: t("单选"),
    description: t("单选磁贴（位置分段）"),
    icon: "mdi:window-shutter",
    category: "control",
    constructor: HuiCoverCard,
  });
  registerCard({
    type: "trigger",
    name: t("按钮"),
    description: t("触发器（收到 true 时短暂亮起）"),
    icon: "mdi:gesture-tap-button",
    category: "control",
    constructor: HuiTriggerCard,
  });
  registerCard({
    type: "switch",
    name: t("开关"),
    description: t("开关磁贴（Flood light 风格）"),
    icon: "mdi:toggle-switch",
    category: "control",
    constructor: HuiSwitchCard,
  });
  registerCard({
    type: "slider",
    name: t("滑块"),
    description: t("亮度滑块（Spotlights 风格）"),
    icon: "mdi:tune-vertical",
    category: "control",
    constructor: HuiSliderCard,
  });
  registerCard({
    type: "gain",
    name: t("增益"),
    description: t("增益调节（dB 步进，点击静音键降至最小值并锁定）"),
    icon: "mdi:volume-high",
    category: "control",
    constructor: HuiGainCard,
  });
  registerCard({
    type: "rgba",
    name: "RGBA",
    description: t("RGBA 颜色选择（绑定 [r,g,b,a] 0–1 地址）"),
    icon: "mdi:palette",
    category: "control",
    constructor: HuiRgbaCard,
  });
  registerCard({
    type: "hsv",
    name: "HSV",
    description: t("HSV 颜色选择（绑定 [h,s,v] 0–1 地址）"),
    icon: "mdi:palette-swatch",
    category: "control",
    constructor: HuiHsvCard,
  });
  // Legacy alias: existing layouts with type "color" still resolve.
  registerCard({
    type: "color",
    name: t("颜色（旧）"),
    description: t("已更名为 RGBA，保留兼容"),
    icon: "mdi:palette",
    category: "control",
    constructor: HuiColorCard,
  });
  registerCard({
    type: "multi-fader",
    name: t("多维推杆"),
    description: t("多个推杆，输出 N 维数组到同一地址"),
    icon: "mdi:tune-vertical-variant",
    category: "control",
    constructor: HuiMultiFaderCard,
  });
  registerCard({
    type: "xy-pad",
    name: t("XY 触控板"),
    description: t("二维触控板（绑定 [x,y] 或分别绑定 X/Y 地址）"),
    icon: "mdi:axis-arrow",
    category: "control",
    constructor: HuiXyPadCard,
  });
  registerCard({
    type: "media",
    name: t("媒体"),
    description: t("播放器启停控制（播放 / 停止）"),
    icon: "mdi:cast-audio",
    category: "control",
    constructor: HuiMediaCard,
  });
  registerCard({
    type: "mic",
    name: t("对讲麦克风"),
    description: t("按住说话，实时 PCM 推送到 Web Mic 节点"),
    icon: "mdi:microphone",
    category: "control",
    constructor: HuiMicCard,
  });
  registerCard({
    type: "markdown",
    name: "Markdown",
    description: t("文本说明卡片"),
    icon: "mdi:text",
    category: "container",
    constructor: HuiMarkdownCard,
  });
  registerCard({
    type: "link",
    name: t("超链接"),
    description: t("点击跳转到外部或内部链接"),
    icon: "mdi:link-variant",
    category: "container",
    constructor: HuiLinkCard,
  });
  registerCard({
    type: "picture-elements",
    name: t("图片元素"),
    description: t("在底图上叠加状态徽章与控制徽章"),
    icon: "mdi:floor-plan",
    category: "container",
    constructor: HuiPictureElementsCard,
  });
  registerCard({
    type: "path-badge",
    name: t("轨迹元素"),
    description: t("在底图上绘制轨迹，并让多个徽章按各自的 0-1 值沿轨迹运动"),
    icon: "mdi:map-marker-path",
    category: "container",
    constructor: HuiPathBadgeCard,
  });
  registerLazyChartCard(
    "line3d",
    t("3D 折线"),
    t("实时三维折线图（订阅 OSC 地址推送的点坐标）"),
    "mdi:chart-timeline-variant",
    () => import("./hui-line3d-card"),
    "HuiLine3dCard",
  );
  registerLazyChartCard(
    "scatter3d",
    t("3D 散点"),
    t("实时三维散点图（订阅 OSC 地址推送的点坐标）"),
    "mdi:chart-scatter-plot",
    () => import("./hui-scatter3d-card"),
    "HuiScatter3dCard",
  );
  registerLazyChartCard(
    "line2d",
    t("2D 折线"),
    t("实时二维折线图（订阅 OSC 地址推送的 [x,y] 坐标）"),
    "mdi:chart-line",
    () => import("./hui-line2d-card"),
    "HuiLine2dCard",
  );
  registerLazyChartCard(
    "scatter2d",
    t("2D 散点"),
    t("实时二维散点图（订阅 OSC 地址推送的 [x,y] 坐标）"),
    "mdi:chart-scatter-plot-hexbin",
    () => import("./hui-scatter2d-card"),
    "HuiScatter2dCard",
  );
  registerLazyChartCard(
    "bar",
    t("柱状图"),
    t("每根柱绑定一个地址，显示该地址的实时数值"),
    "mdi:chart-bar",
    () => import("./hui-bar-card"),
    "HuiBarCard",
  );
  registerCard({
    type: "error",
    name: "Error",
    category: "container",
    constructor: HuiErrorCard,
  });
}

// Eager side-effect registration
registerBuiltinCards();
