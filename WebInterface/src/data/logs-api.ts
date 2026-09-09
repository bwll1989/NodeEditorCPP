export interface FlowLogEntry {
  seq: number;
  timestamp: string;
  level: string;
  message: string;
}

export type LogLevelFilter = "All" | "Debug" | "Info" | "Warn" | "Critical" | "Fatal";

export async function fetchLogTail(options?: {
  limit?: number;
  level?: LogLevelFilter;
  since?: number;
}): Promise<{ ok: boolean; items: FlowLogEntry[]; error?: string }> {
  const params = new URLSearchParams();
  const limit = options?.limit ?? 300;
  params.set("limit", String(limit));
  if (options?.level && options.level !== "All") {
    params.set("level", options.level);
  }
  if (options?.since && options.since > 0) {
    params.set("since", String(options.since));
  }

  try {
    const res = await fetch(`/api/logs/tail?${params.toString()}`);
    const data = (await res.json()) as {
      ok?: boolean;
      items?: FlowLogEntry[];
      error?: string;
    };
    if (data.ok === false) {
      return { ok: false, items: [], error: data.error || "加载失败" };
    }
    const items = Array.isArray(data.items) ? data.items : [];
    return { ok: true, items };
  } catch {
    return { ok: false, items: [], error: "无法连接 Flow 日志服务" };
  }
}

export function levelClass(level: string): string {
  switch (level) {
    case "Debug":
      return "level-debug";
    case "Warn":
      return "level-warn";
    case "Critical":
    case "Fatal":
      return "level-critical";
    default:
      return "level-info";
  }
}

export interface FlowLogFile {
  name: string;
  size: number;
  modified?: string;
}

export async function fetchLogFiles(): Promise<{
  ok: boolean;
  items: FlowLogFile[];
  error?: string;
}> {
  try {
    const res = await fetch("/api/logs/files");
    const data = (await res.json()) as {
      ok?: boolean;
      items?: FlowLogFile[];
      error?: string;
    };
    if (data.ok === false) {
      return { ok: false, items: [], error: data.error || "加载失败" };
    }
    return { ok: true, items: Array.isArray(data.items) ? data.items : [] };
  } catch {
    return { ok: false, items: [], error: "无法连接 Flow 日志服务" };
  }
}

export async function downloadLogFile(fileName: string): Promise<boolean> {
  try {
    const params = new URLSearchParams({ file: fileName });
    const res = await fetch(`/api/logs/download?${params.toString()}`);
    if (!res.ok) {
      return false;
    }
    const blob = await res.blob();
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = fileName;
    anchor.click();
    URL.revokeObjectURL(url);
    return true;
  } catch {
    return false;
  }
}

export function formatLogFileSize(bytes: number): string {
  if (!Number.isFinite(bytes) || bytes <= 0) return "0 B";
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

export function formatLogFileTime(iso?: string): string {
  if (!iso) return "—";
  const date = new Date(iso);
  if (Number.isNaN(date.getTime())) return iso;
  return date.toLocaleString("zh-CN", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
}
