export interface ConfirmDialogParams {
  title: string;
  text?: string;
  confirmText?: string;
  cancelText?: string;
  destructive?: boolean;
}

type ConfirmResolver = (value: boolean) => void;

let confirmResolver: ConfirmResolver | null = null;
let deleteCardResolver: ConfirmResolver | null = null;

export function showConfirmationDialog(params: ConfirmDialogParams): Promise<boolean> {
  return new Promise((resolve) => {
    confirmResolver = resolve;
    document.dispatchEvent(
      new CustomEvent("flow-show-confirm", {
        bubbles: true,
        composed: true,
        detail: params,
      }),
    );
  });
}

export function resolveConfirmationDialog(confirmed: boolean): void {
  confirmResolver?.(confirmed);
  confirmResolver = null;
}

export function showDeleteCardDialog(
  cardConfig?: import("../types").LovelaceCardConfig,
): Promise<boolean> {
  return new Promise((resolve) => {
    deleteCardResolver = resolve;
    document.dispatchEvent(
      new CustomEvent("flow-show-delete-card", {
        bubbles: true,
        composed: true,
        detail: { cardConfig },
      }),
    );
  });
}

export function resolveDeleteCardDialog(confirmed: boolean): void {
  deleteCardResolver?.(confirmed);
  deleteCardResolver = null;
}
