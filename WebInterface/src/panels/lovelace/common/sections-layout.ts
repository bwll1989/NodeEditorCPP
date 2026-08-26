import type { LovelaceSectionConfig } from "../../../types";

/** Port of HA `DEFAULT_MAX_COLUMNS` (hui-sections-view) */
export const DEFAULT_MAX_COLUMNS = 4;

/** Port of HA `--base-column-count` in hui-grid-section */
export const SECTION_BASE_COLUMN_COUNT = 12;

export function parseCssPx(value: string): number {
  return parseInt(value.replace("px", "").trim(), 10) || 0;
}

/** Sum section `column_span` values (HA `_sectionColumnCount`). */
export function sumSectionColumnSpans(sections: LovelaceSectionConfig[]): number {
  return sections.reduce((acc, section) => acc + (section.column_span ?? 1), 0);
}

/**
 * Responsive viewport column cap (HA ResizeController + max_columns clamp).
 * @param configMaxColumns view.max_columns, default 4
 */
export function computeResponsiveMaxColumns(
  totalWidth: number,
  options: {
    minColumnWidth?: number;
    columnGap?: number;
    horizontalPadding?: number;
    configMaxColumns?: number;
  } = {},
): number {
  const minColumnWidth = options.minColumnWidth ?? 320;
  const columnGap = options.columnGap ?? 32;
  const horizontalPadding = options.horizontalPadding ?? 0;
  const configMax = options.configMaxColumns ?? DEFAULT_MAX_COLUMNS;

  const columns = Math.floor(
    (totalWidth - horizontalPadding + columnGap) / (minColumnWidth + columnGap),
  );
  return Math.max(1, Math.min(configMax, columns));
}

/**
 * View grid column count (HA hui-sections-view).
 * Uses total column-span units, not raw section count.
 */
export function computeViewGridColumnCount(options: {
  sectionColumnSpanSum: number;
  maxColumns: number;
  editMode?: boolean;
  extraSlots?: number;
}): number {
  const { sectionColumnSpanSum, maxColumns, editMode = false, extraSlots = 0 } = options;
  const totalSlots = sectionColumnSpanSum + (editMode ? 1 : 0) + extraSlots;
  return Math.max(Math.min(maxColumns, totalSlots), 1);
}

export function clampSectionColumnSpan(span: number | undefined, contentColumnCount: number): number {
  return Math.min(span ?? 1, contentColumnCount);
}

/** Internal card grid columns: 12 × section column_span (HA hui-grid-section). */
export function sectionInternalGridColumns(columnSpan: number): number {
  return SECTION_BASE_COLUMN_COUNT * Math.max(1, columnSpan);
}
