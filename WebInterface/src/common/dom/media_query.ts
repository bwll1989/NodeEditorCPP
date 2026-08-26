/** Port of HA `listenMediaQuery` */
export function listenMediaQuery(
  query: string,
  callback: (matches: boolean) => void,
): () => void {
  const mediaQuery = window.matchMedia(query);
  const listener = (ev: MediaQueryListEvent) => callback(ev.matches);
  mediaQuery.addEventListener("change", listener);
  callback(mediaQuery.matches);
  return () => mediaQuery.removeEventListener("change", listener);
}
