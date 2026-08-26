/** Coordinate edit menus so only one stays open and hover states don't fight. */
export const EDIT_MENU_OPEN = "flow-edit-menu-open";

export function notifyEditMenuOpen(source: HTMLElement): void {
  document.dispatchEvent(
    new CustomEvent(EDIT_MENU_OPEN, {
      bubbles: true,
      composed: true,
      detail: { source },
    }),
  );
}

export function onEditMenuOpen(handler: (source: HTMLElement) => void): () => void {
  const listener = (ev: Event): void => {
    const source = (ev as CustomEvent<{ source: HTMLElement }>).detail?.source;
    if (source) handler(source);
  };
  document.addEventListener(EDIT_MENU_OPEN, listener);
  return () => document.removeEventListener(EDIT_MENU_OPEN, listener);
}
