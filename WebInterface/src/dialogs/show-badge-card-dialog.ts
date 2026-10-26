import type { Flow, LovelaceCardConfig } from "../types";

export interface BadgeCardDialogParams {
  flow?: Flow;
  cardConfig: LovelaceCardConfig;
  title?: string;
}

type CloseResolver = () => void;

let closeResolver: CloseResolver | null = null;

/** 弹出徽章绑定的卡片对话框。 */
export function showBadgeCardDialog(params: BadgeCardDialogParams): Promise<void> {
  return new Promise((resolve) => {
    closeResolver = resolve;
    document.dispatchEvent(
      new CustomEvent("flow-show-badge-card", {
        bubbles: true,
        composed: true,
        detail: params,
      }),
    );
  });
}

export function resolveBadgeCardDialog(): void {
  closeResolver?.();
  closeResolver = null;
}
