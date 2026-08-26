export interface FixedDropdownPosition {
  top: number;
  left: number;
  width: number;
  maxHeight: number;
}

export interface MenuSize {
  width: number;
  height: number;
}

export interface AnchoredMenuPosition extends FixedDropdownPosition {
  placement: "bottom-end" | "top-end";
}

export interface AnchoredMenuOptions {
  preferredMaxHeight?: number;
  minMenuWidth?: number;
  margin?: number;
  gap?: number;
}

/** HA-style bottom-end / top-end placement with flip when space is tight. */
export function computeAnchoredMenuPosition(
  triggerRect: DOMRect,
  menuSize: MenuSize,
  options: AnchoredMenuOptions = {},
): AnchoredMenuPosition {
  const margin = options.margin ?? 8;
  const gap = options.gap ?? 4;
  const preferredMaxHeight = options.preferredMaxHeight ?? 360;
  const minMenuWidth = options.minMenuWidth ?? 160;
  const menuWidth = Math.max(minMenuWidth, menuSize.width);

  const spaceBelow = window.innerHeight - triggerRect.bottom - margin;
  const spaceAbove = triggerRect.top - margin;
  const contentHeight = Math.max(menuSize.height, 1);

  const openBelow = spaceBelow >= contentHeight + gap || spaceBelow >= spaceAbove;
  const placement: "bottom-end" | "top-end" = openBelow ? "bottom-end" : "top-end";

  const available = openBelow ? spaceBelow - gap : spaceAbove - gap;
  const maxHeight = Math.max(120, Math.min(preferredMaxHeight, available));
  const visibleHeight = Math.min(contentHeight, maxHeight);

  let top = openBelow
    ? triggerRect.bottom + gap
    : triggerRect.top - visibleHeight - gap;
  top = Math.max(margin, top);

  let left = triggerRect.right - menuWidth;
  left = Math.max(margin, Math.min(left, window.innerWidth - menuWidth - margin));

  return {
    top,
    left,
    width: menuWidth,
    maxHeight,
    placement,
  };
}

/** Start-aligned picker menus (icon/color pickers). */
export function computeFixedDropdownPosition(
  trigger: HTMLElement,
  preferredMaxHeight = 320,
): FixedDropdownPosition {
  const rect = trigger.getBoundingClientRect();
  const margin = 8;
  const gap = 4;
  const menuWidth = Math.max(rect.width, 160);
  const estimatedHeight = 180;
  const spaceBelow = window.innerHeight - rect.bottom - margin;
  const spaceAbove = rect.top - margin;
  const openBelow = spaceBelow >= estimatedHeight + gap || spaceBelow >= spaceAbove;
  const maxHeight = Math.max(
    120,
    Math.min(preferredMaxHeight, (openBelow ? spaceBelow : spaceAbove) - gap),
  );
  const visibleHeight = Math.min(estimatedHeight, maxHeight);
  const top = openBelow
    ? rect.bottom + gap
    : Math.max(margin, rect.top - visibleHeight - gap);

  return {
    top,
    left: rect.left,
    width: menuWidth,
    maxHeight,
  };
}
