export interface AppInfo {
  name: string;
  version: string;
}

export async function fetchAppInfo(): Promise<AppInfo> {
  try {
    const resp = await fetch("/api/info/app");
    if (!resp.ok) return { name: "Flow", version: "未知" };
    const json = (await resp.json()) as { ok?: boolean; name?: string; version?: string };
    if (json.ok) {
      return {
        name: json.name ? String(json.name) : "Flow",
        version: json.version ? String(json.version) : "未知",
      };
    }
    return { name: "Flow", version: "未知" };
  } catch {
    return { name: "Flow", version: "获取失败" };
  }
}

export async function fetchCurrentFlowName(): Promise<string> {
  try {
    const resp = await fetch("/api/info/current_flow");
    if (!resp.ok) return "获取失败";
    const json = (await resp.json()) as { ok?: boolean; filename?: string };
    return json.ok && json.filename ? String(json.filename) : "无项目运行";
  } catch {
    return "连接异常";
  }
}

export function uploadFileBinary(
  url: string,
  file: Blob,
  onProgress?: (percent: number) => void,
): Promise<{ ok: true; path: string }> {
  return new Promise((resolve, reject) => {
    if (!file || typeof file.size !== "number") {
      reject(new Error("invalid_file"));
      return;
    }
    const xhr = new XMLHttpRequest();
    xhr.open("POST", url, true);
    xhr.setRequestHeader("Content-Type", "application/octet-stream");
    xhr.upload.onprogress = (e) => {
      if (!onProgress) return;
      if (e.lengthComputable && e.total > 0) {
        onProgress(Math.round((e.loaded / e.total) * 100));
      } else if (file.size > 0) {
        onProgress(Math.min(99, Math.round((e.loaded / file.size) * 100)));
      }
    };
    xhr.onload = () => {
      const text = xhr.responseText || "";
      let json: { ok?: boolean; path?: string; error?: string } | null = null;
      try {
        json = JSON.parse(text) as { ok?: boolean; path?: string; error?: string };
      } catch {
        json = null;
      }
      if (xhr.status >= 200 && xhr.status < 300 && json?.ok) {
        onProgress?.(100);
        resolve({ ok: true, path: json.path || "" });
        return;
      }
      reject(new Error(json?.error ?? `${xhr.status} ${text}`));
    };
    xhr.onerror = () => reject(new Error("network_error"));
    xhr.onabort = () => reject(new Error("aborted"));
    xhr.send(file);
  });
}

export async function downloadCurrentFlow(): Promise<{ filename: string; blob: Blob }> {
  const resp = await fetch("/api/download/current_flow");
  if (!resp.ok) {
    const json = (await resp.json().catch(() => ({}))) as { error?: string };
    throw new Error(json.error || resp.statusText);
  }
  const blob = await resp.blob();
  const disposition = resp.headers.get("Content-Disposition") || "";
  let filename = "project.flow";
  const matches = /filename="([^"]*)"/.exec(disposition);
  if (matches?.[1]) filename = matches[1];
  return { filename, blob };
}

export const MEDIA_ACCEPT =
  "video/*,audio/*,image/*,.mp4,.mov,.avi,.mkv,.mp3,.wav,.png,.jpg,.jpeg,.gif";
