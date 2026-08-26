export function fireEvent<T extends keyof HASSDomEvents>(
  el: HTMLElement,
  type: T,
  detail?: HASSDomEvents[T],
): void {
  el.dispatchEvent(
    new CustomEvent(type, {
      detail,
      bubbles: true,
      composed: true,
    }),
  );
}
