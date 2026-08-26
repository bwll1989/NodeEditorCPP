const AUTH_TS_KEY = "ns_setting_auth_ts";
/** Session unlock window after a successful password check. */
const AUTH_TTL_MS = 10 * 60 * 1000;

export type AuthVerifyResult = "ok" | "deny" | "offline";

export interface AuthPromptParams {
  title?: string;
  hint?: string;
  error?: string;
}

export function markSettingAuthed(): void {
  try {
    sessionStorage.setItem(AUTH_TS_KEY, String(Date.now()));
  } catch {
    // ignore
  }
}

export function clearSettingAuthed(): void {
  try {
    sessionStorage.removeItem(AUTH_TS_KEY);
  } catch {
    // ignore
  }
}

export function isSettingAuthedRecently(): boolean {
  try {
    const ts = Number(sessionStorage.getItem(AUTH_TS_KEY) || 0);
    return ts > 0 && Date.now() - ts < AUTH_TTL_MS;
  } catch {
    return false;
  }
}

export async function checkSettingPassword(password: string): Promise<AuthVerifyResult> {
  try {
    const resp = await fetch("/api/auth/setting", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ password: password || "" }),
    });
    const json = (await resp.json().catch(() => null)) as { ok?: boolean } | null;
    return resp.ok && json?.ok ? "ok" : "deny";
  } catch {
    // No Flow HTTP server (static preview / offline) — treat as unlocked.
    return "offline";
  }
}

export async function verifySettingPassword(password: string): Promise<boolean> {
  const result = await checkSettingPassword(password);
  return result === "ok" || result === "offline";
}

type PasswordResolver = (value: string | null) => void;

let passwordResolver: PasswordResolver | null = null;

/** Show the host password dialog; resolves to password or null if cancelled. */
export function promptSettingPassword(params: AuthPromptParams = {}): Promise<string | null> {
  return new Promise((resolve) => {
    passwordResolver = resolve;
    document.dispatchEvent(
      new CustomEvent("flow-show-auth", {
        bubbles: true,
        composed: true,
        detail: params,
      }),
    );
  });
}

export function resolveSettingPasswordPrompt(password: string | null): void {
  passwordResolver?.(password);
  passwordResolver = null;
}

/**
 * Gate dashboard edit / settings mutations.
 * - Recent session unlock → pass
 * - Empty server password (or offline) → pass
 * - Otherwise prompt until success or cancel
 */
export async function requireEditAuth(options?: AuthPromptParams): Promise<boolean> {
  if (isSettingAuthedRecently()) return true;

  const emptyCheck = await checkSettingPassword("");
  if (emptyCheck === "ok" || emptyCheck === "offline") {
    markSettingAuthed();
    return true;
  }

  let lastError = options?.error ?? "";
  while (true) {
    const password = await promptSettingPassword({
      title: options?.title ?? "进入编辑模式",
      hint: options?.hint ?? "请输入编辑口令。验证通过后，本会话 10 分钟内无需重复输入。",
      error: lastError,
    });
    if (password === null) return false;

    const result = await checkSettingPassword(password);
    if (result === "ok" || result === "offline") {
      markSettingAuthed();
      return true;
    }
    lastError = "密码错误，请重试";
  }
}
