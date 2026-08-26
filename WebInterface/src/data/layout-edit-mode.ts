const LAYOUT_EDIT_ENABLED_KEY = "flow_layout_edit_enabled";

export const LAYOUT_EDIT_ENABLED_EVENT = "flow-layout-edit-enabled-changed";

export function isLayoutEditEnabled(): boolean {
  try {
    return localStorage.getItem(LAYOUT_EDIT_ENABLED_KEY) === "true";
  } catch {
    return false;
  }
}

export function setLayoutEditEnabled(enabled: boolean): void {
  try {
    localStorage.setItem(LAYOUT_EDIT_ENABLED_KEY, enabled ? "true" : "false");
  } catch {
    // ignore
  }
  document.dispatchEvent(
    new CustomEvent(LAYOUT_EDIT_ENABLED_EVENT, {
      detail: { enabled },
    }),
  );
}
