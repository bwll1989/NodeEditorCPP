/**
 * 图表（ECharts Canvas）主题色读取工具。
 *
 * Canvas 无法直接使用 CSS 变量，这里在构建 option 时从宿主元素
 * 的 computedStyle 读取语义变量，暗色主题下自动获得亮色轴系。
 */

export interface ChartThemeColors {
  axisLine: string;
  axisLabel: string;
  axisName: string;
  splitLine: string;
}

const FALLBACK: ChartThemeColors = {
  axisLine: "rgba(0, 0, 0, 0.2)",
  axisLabel: "rgba(0, 0, 0, 0.54)",
  axisName: "rgba(0, 0, 0, 0.6)",
  splitLine: "rgba(0, 0, 0, 0.08)",
};

export function readChartThemeColors(el: HTMLElement | null | undefined): ChartThemeColors {
  if (!el) return FALLBACK;
  try {
    const styles = getComputedStyle(el);
    const read = (name: string, fallback: string): string => {
      const value = styles.getPropertyValue(name).trim();
      return value || fallback;
    };
    return {
      axisLine: read("--chart-axis-line-color", read("--divider-color", FALLBACK.axisLine)),
      axisLabel: read("--chart-axis-label-color", read("--secondary-text-color", FALLBACK.axisLabel)),
      axisName: read("--chart-axis-name-color", read("--secondary-text-color", FALLBACK.axisName)),
      splitLine: read("--chart-split-line-color", read("--divider-color", FALLBACK.splitLine)),
    };
  } catch {
    return FALLBACK;
  }
}

/**
 * 监听 <html data-theme> 变化（亮/暗切换时重渲染图表）。
 * 返回取消监听函数。
 */
export function observeThemeChanges(el: HTMLElement, onChange: () => void): () => void {
  const observer = new MutationObserver(() => onChange());
  observer.observe(document.documentElement, {
    attributes: true,
    attributeFilter: ["data-theme"],
  });
  el.addEventListener("disconnected", () => observer.disconnect(), { once: true });
  return () => observer.disconnect();
}
