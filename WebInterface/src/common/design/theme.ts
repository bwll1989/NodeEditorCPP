/**
 * Flow 主题模式管理
 *
 * 模式：
 *   "auto"  —— 跟随系统 prefers-color-scheme（默认）
 *   "light" —— 固定亮色
 *   "dark"  —— 固定暗色
 *
 * 实际生效主题写入 <html data-theme="light|dark">，由 global.css 的
 * :root[data-theme="dark"] 块接管全部语义变量。
 */

export type ThemeMode = "auto" | "light" | "dark";
export type ResolvedTheme = "light" | "dark";

const THEME_MODE_KEY = "flow_theme_mode";
const SYSTEM_DARK_QUERY = "(prefers-color-scheme: dark)";

function readStoredMode(): ThemeMode {
  try {
    const value = localStorage.getItem(THEME_MODE_KEY);
    if (value === "light" || value === "dark" || value === "auto") return value;
  } catch {
    // ignore
  }
  return "auto";
}

export function getSystemTheme(): ResolvedTheme {
  return window.matchMedia?.(SYSTEM_DARK_QUERY).matches ? "dark" : "light";
}

export function resolveTheme(mode: ThemeMode): ResolvedTheme {
  return mode === "auto" ? getSystemTheme() : mode;
}

/** 将生效主题应用到 <html data-theme="...">。 */
export function applyResolvedTheme(theme: ResolvedTheme): void {
  document.documentElement.setAttribute("data-theme", theme);
}

export function getThemeMode(): ThemeMode {
  return readStoredMode();
}

export function getActiveTheme(): ResolvedTheme {
  return resolveTheme(readStoredMode());
}

/** 设置主题模式并立即应用；mode 传 undefined 表示重置为跟随系统。 */
export function setThemeMode(mode: ThemeMode): void {
  try {
    localStorage.setItem(THEME_MODE_KEY, mode);
  } catch {
    // ignore
  }
  applyResolvedTheme(resolveTheme(mode));
}

/**
 * 初始化主题：应用当前偏好，并在 auto 模式下监听系统主题变化。
 * 返回取消监听的函数（应用生命周期内一般无需调用）。
 */
export function initTheme(): () => void {
  applyResolvedTheme(resolveTheme(readStoredMode()));

  const media = window.matchMedia?.(SYSTEM_DARK_QUERY);
  if (!media) return () => undefined;
  const onChange = (): void => {
    if (readStoredMode() === "auto") {
      applyResolvedTheme(getSystemTheme());
    }
  };
  media.addEventListener("change", onChange);
  return () => media.removeEventListener("change", onChange);
}

/**
 * 切换主题：保存与当前生效主题相反的固定偏好。
 * （初始为 auto 跟随系统；用户点击后固定为 light/dark，再点切回另一侧。）
 */
export function toggleTheme(): ResolvedTheme {
  const next: ResolvedTheme = getActiveTheme() === "dark" ? "light" : "dark";
  setThemeMode(next);
  return next;
}
