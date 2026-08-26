/**
 * 仅在遮罩上按下并松开时才关闭弹窗，避免在输入框内拖选文本时误触关闭。
 */
export function createBackdropDismissHandlers(onDismiss: () => void) {
  let pointerDownOnBackdrop = false;

  const onBackdropPointerDown = (ev: PointerEvent): void => {
    pointerDownOnBackdrop = ev.target === ev.currentTarget;
  };

  const onBackdropPointerUp = (ev: PointerEvent): void => {
    if (pointerDownOnBackdrop && ev.target === ev.currentTarget) {
      onDismiss();
    }
    pointerDownOnBackdrop = false;
  };

  const onBackdropPointerCancel = (): void => {
    pointerDownOnBackdrop = false;
  };

  return {
    onBackdropPointerDown,
    onBackdropPointerUp,
    onBackdropPointerCancel,
  };
}
