import type {
  LovelaceCardConfig,
  LovelaceConfig,
  LovelaceSectionConfig,
  LovelaceViewConfig,
} from "../types";

export const DEFAULT_SECTION_BACKGROUND_OPACITY = 50;

/** 新建标题卡片的默认配置（与 HA Title 样式一致） */
export const DEFAULT_HEADING_CARD_CONFIG: LovelaceCardConfig = {
  type: "heading",
  heading: "新建部件",
  heading_style: "title",
  badges: [
    {
      type: "button",
      entity: "",
      icon: "mdi:gesture-tap-button",
      text: "触发",
      value: true,
      show_name: true,
    },
  ],
};

export interface LovelaceSectionBackgroundConfig {
  color?: string;
  opacity?: number;
}

export function resolveSectionBackground(
  background?: boolean | LovelaceSectionBackgroundConfig,
): LovelaceSectionBackgroundConfig | undefined {
  if (background === undefined) return undefined;
  if (typeof background === "boolean") return background ? {} : undefined;
  return background;
}

export function generateDefaultSection(includeHeading = true): LovelaceSectionConfig {
  return {
    type: "grid",
    cards: includeHeading ? [{ ...DEFAULT_HEADING_CARD_CONFIG }] : [],
  };
}

export function generateDefaultView(
  options?: { title?: string; path?: string; icon?: string },
): LovelaceViewConfig {
  return {
    title: options?.title ?? "新建视图",
    path: options?.path ?? "view",
    icon: options?.icon ?? "mdi:view-dashboard",
    max_columns: 4,
    sections: [generateDefaultSection(true)],
  };
}

/** Migrate legacy section.title/icon/badges into a heading card */
export function migrateCard(card: LovelaceCardConfig): LovelaceCardConfig {
  if (card.type === "button") {
    return { ...card, type: "trigger" };
  }
  if (card.type === "color") {
    return { ...card, type: "rgba" };
  }
  if (card.type === "bar" && card.entity && !card.entities) {
    const { entity, name, ...rest } = card as LovelaceCardConfig & { entity?: string; name?: string };
    return {
      ...rest,
      type: "bar",
      entities: [{ entity: String(entity), name: name ? String(name) : undefined }],
    };
  }
  return card;
}

export function migrateSection(section: LovelaceSectionConfig): LovelaceSectionConfig {
  const { title, icon, badges, ...rest } = section as LovelaceSectionConfig & {
    title?: string;
    icon?: string;
    badges?: { entity?: string; name?: string; icon?: string; type?: string }[];
  };

  const cards = [...(rest.cards ?? [])].map((card) => migrateCard(card));
  const needsHeading = Boolean(title || icon || badges?.length);
  const hasHeading = cards.some((c) => c.type === "heading");

  if (needsHeading && !hasHeading) {
    cards.unshift({
      type: "heading",
      heading: title || "新建部件",
      icon,
      badges: badges?.map((b) => ({
        type: b.type ?? "entity",
        entity: b.entity,
        name: b.name,
        icon: b.icon,
      })),
    });
  }

  return {
    type: "grid",
    column_span: rest.column_span,
    row_span: rest.row_span,
    background: rest.background,
    theme: rest.theme,
    cards,
  };
}

export function migrateConfig(config: LovelaceConfig): LovelaceConfig {
  return {
    ...config,
    views: config.views.map((view) => migrateView(view)),
  };
}

export function migrateView(view: LovelaceViewConfig): LovelaceViewConfig {
  return {
    ...view,
    sections: view.sections.map((s) => migrateSection(s)),
  };
}

export function collectCardEntities(card: LovelaceCardConfig): string[] {
  const set = new Set<string>();
  if (card.entity) set.add(card.entity);
  const muteEntity = card.mute_entity;
  if (typeof muteEntity === "string" && muteEntity) set.add(muteEntity);
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
  return [...set];
}
