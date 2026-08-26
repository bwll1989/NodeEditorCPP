import type { LovelaceCardConfig, LovelaceConfig, LovelaceSectionConfig, LovelaceViewConfig } from "../types";

export interface CardPath {
  viewIndex: number;
  sectionIndex: number;
  cardIndex: number;
}

function moveInArray<T>(items: T[], oldIndex: number, newIndex: number): T[] {
  const next = [...items];
  const [item] = next.splice(oldIndex, 1);
  next.splice(newIndex, 0, item);
  return next;
}

export function getActiveView(config: LovelaceConfig, viewIndex: number): LovelaceViewConfig {
  return config.views[viewIndex];
}

/** Replace a section immutably so Lit detects config changes */
export function replaceSection(
  view: LovelaceViewConfig,
  sectionIndex: number,
  section: LovelaceSectionConfig,
): void {
  view.sections = view.sections.map((item, index) =>
    index === sectionIndex ? section : item,
  );
}

export function addSection(view: LovelaceViewConfig, section: LovelaceSectionConfig): void {
  view.sections = [...view.sections, section];
}

export function addCard(
  section: LovelaceSectionConfig,
  card: LovelaceCardConfig,
): LovelaceSectionConfig {
  return { ...section, cards: [...section.cards, card] };
}

export function cardPath(viewIndex: number, sectionIndex: number, cardIndex: number): CardPath {
  return { viewIndex, sectionIndex, cardIndex };
}

export function parseCardPath(data: unknown): CardPath | undefined {
  if (Array.isArray(data) && data.length === 3) {
    const [viewIndex, sectionIndex, cardIndex] = data;
    if (
      typeof viewIndex === "number" &&
      typeof sectionIndex === "number" &&
      typeof cardIndex === "number"
    ) {
      return { viewIndex, sectionIndex, cardIndex };
    }
  }
  if (data && typeof data === "object") {
    const record = data as Partial<CardPath>;
    if (
      typeof record.viewIndex === "number" &&
      typeof record.sectionIndex === "number" &&
      typeof record.cardIndex === "number"
    ) {
      return record as CardPath;
    }
  }
  return undefined;
}

export function moveCardInView(
  view: LovelaceViewConfig,
  fromSectionIndex: number,
  fromCardIndex: number,
  toSectionIndex: number,
  toCardIndex: number,
): void {
  const fromSection = view.sections[fromSectionIndex];
  const toSection = view.sections[toSectionIndex];
  if (!fromSection || !toSection) return;

  const card = fromSection.cards[fromCardIndex];
  if (!card) return;

  if (fromSectionIndex === toSectionIndex) {
    replaceSection(view, fromSectionIndex, {
      ...fromSection,
      cards: moveInArray(fromSection.cards, fromCardIndex, toCardIndex),
    });
    return;
  }

  const fromCards = fromSection.cards.filter((_, index) => index !== fromCardIndex);
  const toCards = [...toSection.cards];
  const insertIndex = Math.max(0, Math.min(toCardIndex, toCards.length));
  toCards.splice(insertIndex, 0, card);

  replaceSection(view, fromSectionIndex, { ...fromSection, cards: fromCards });
  replaceSection(view, toSectionIndex, { ...toSection, cards: toCards });
}

export function moveCardInSection(
  section: LovelaceSectionConfig,
  oldIndex: number,
  newIndex: number,
): void {
  section.cards = moveInArray(section.cards, oldIndex, newIndex);
}

export function moveSectionInView(
  view: LovelaceViewConfig,
  oldIndex: number,
  newIndex: number,
): void {
  view.sections = moveInArray(view.sections, oldIndex, newIndex);
}

export function duplicateCard(
  section: LovelaceSectionConfig,
  cardIndex: number,
): LovelaceSectionConfig {
  const source = section.cards[cardIndex];
  if (!source) return section;
  const clone = structuredClone(source) as LovelaceCardConfig;
  const cards = [...section.cards];
  cards.splice(cardIndex + 1, 0, clone);
  return { ...section, cards };
}

export function deleteCard(
  section: LovelaceSectionConfig,
  cardIndex: number,
): LovelaceSectionConfig {
  return {
    ...section,
    cards: section.cards.filter((_, index) => index !== cardIndex),
  };
}

export function duplicateSection(view: LovelaceViewConfig, sectionIndex: number): void {
  const source = view.sections[sectionIndex];
  if (!source) return;
  const clone = structuredClone(source) as LovelaceSectionConfig;
  const sections = [...view.sections];
  sections.splice(sectionIndex + 1, 0, clone);
  view.sections = sections;
}

export function deleteSection(view: LovelaceViewConfig, sectionIndex: number): void {
  view.sections = view.sections.filter((_, index) => index !== sectionIndex);
}

export function updateCard(
  section: LovelaceSectionConfig,
  cardIndex: number,
  patch: LovelaceCardConfig,
): LovelaceSectionConfig {
  const card = section.cards[cardIndex];
  if (!card) return section;
  const cards = section.cards.map((item, index) =>
    index === cardIndex ? { ...patch } : item,
  );
  return { ...section, cards };
}

export function updateSection(
  view: LovelaceViewConfig,
  sectionIndex: number,
  patch: Partial<LovelaceSectionConfig>,
): void {
  const section = view.sections[sectionIndex];
  if (!section) return;
  const next = { ...section, ...patch };
  if (patch.background === undefined && "background" in patch) {
    delete next.background;
  }
  replaceSection(view, sectionIndex, next);
}

/** Resolve view index from URL path segment (HA: path or numeric index). */
export function resolveViewIndex(
  config: LovelaceConfig,
  pathOrIndex: string | number | undefined,
): number {
  const views = config.views;
  if (!views.length) return 0;
  if (pathOrIndex === undefined || pathOrIndex === "") return 0;
  if (typeof pathOrIndex === "number") {
    return Math.max(0, Math.min(pathOrIndex, views.length - 1));
  }
  const asInt = Number(pathOrIndex);
  for (let i = 0; i < views.length; i++) {
    if (views[i].path === pathOrIndex || i === asInt) return i;
  }
  return 0;
}

export function viewUrlPath(view: LovelaceViewConfig, index: number): string {
  return view.path || String(index);
}

export function generateUniqueViewPath(
  views: LovelaceViewConfig[],
  base = "view",
): string {
  const used = new Set(
    views.flatMap((view, index) => [view.path, String(index)].filter(Boolean) as string[]),
  );
  let n = views.length + 1;
  let path = `${base}-${n}`;
  while (used.has(path)) {
    n += 1;
    path = `${base}-${n}`;
  }
  return path;
}

export function addView(
  config: LovelaceConfig,
  view: LovelaceViewConfig,
  atIndex?: number,
): number {
  const views = [...config.views];
  const index =
    atIndex === undefined
      ? views.length
      : Math.max(0, Math.min(atIndex, views.length));
  views.splice(index, 0, view);
  config.views = views;
  return index;
}

export function deleteView(config: LovelaceConfig, viewIndex: number): number {
  if (config.views.length <= 1) return 0;
  config.views = config.views.filter((_, index) => index !== viewIndex);
  return Math.min(viewIndex, config.views.length - 1);
}

export function moveView(
  config: LovelaceConfig,
  oldIndex: number,
  newIndex: number,
): void {
  config.views = moveInArray(config.views, oldIndex, newIndex);
}

export function updateView(
  config: LovelaceConfig,
  viewIndex: number,
  patch: Partial<LovelaceViewConfig>,
): void {
  const view = config.views[viewIndex];
  if (!view) return;
  config.views = config.views.map((item, index) =>
    index === viewIndex ? { ...item, ...patch } : item,
  );
}
