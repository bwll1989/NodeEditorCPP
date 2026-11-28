// WebInterface 界面多语言（2026-11-28）
// 源文语言=中文：模板/数据中的中文串是查表 key，en 词典提供英文译文，
// 缺失时回退中文原样显示。语言存 localStorage("flow.web.lang")，
// 首次访问按浏览器语言自动选择；切换后整页刷新生效（Lit 无 retranslate 链路）。
import { en } from "./translations";

export type WebLang = "zh" | "en";

const LANG_KEY = "flow.web.lang";

export function getLang(): WebLang {
  const saved = localStorage.getItem(LANG_KEY);
  if (saved === "zh" || saved === "en") return saved;
  // 未设置过：按浏览器偏好（en 开头→英文，其余中文）
  return navigator.language && navigator.language.toLowerCase().startsWith("en")
    ? "en"
    : "zh";
}

/** 翻译：en 语言查词典，缺失或 zh 直接返回源文（key=中文源文） */
export function t(zh: string): string {
  if (getLang() !== "en") return zh;
  return en[zh] ?? zh;
}

/** 带参数的翻译：t("已加载 %1 个场景", 3) */
export function tf(zh: string, ...args: unknown[]): string {
  let s = t(zh);
  for (let i = 0; i < args.length; i++) {
    s = s.replace(`%${i + 1}`, String(args[i]));
  }
  return s;
}

/** 切换语言：持久化并整页刷新（切换入口在设置面板） */
export function setLang(lang: WebLang): void {
  localStorage.setItem(LANG_KEY, lang);
  location.reload();
}
