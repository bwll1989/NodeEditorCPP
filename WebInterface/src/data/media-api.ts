export interface FlowMediaFile {
  name: string;
  size: number;
  modified?: string;
}

export type MediaCategory =
  | "video"
  | "audio"
  | "dmx"
  | "image"
  | "model"
  | "document"
  | "childflow"
  | "unknown";

export interface MediaFileGroup {
  category: MediaCategory;
  label: string;
  items: FlowMediaFile[];
}

const MEDIA_CATEGORY_ORDER: MediaCategory[] = [
  "video",
  "audio",
  "dmx",
  "image",
  "model",
  "document",
  "childflow",
  "unknown",
];

const MEDIA_CATEGORY_LABELS: Record<MediaCategory, string> = {
  video: "视频",
  audio: "音频",
  dmx: "DMX",
  image: "图片",
  model: "3D 模型",
  document: "文档",
  childflow: "子流程",
  unknown: "其他",
};

const VIDEO_EXT = new Set(["mp4", "mov", "mkv", "avi", "wmv", "flv", "webm"]);
const AUDIO_EXT = new Set(["wav", "mp3", "flac", "aac", "ogg", "m4a"]);
const DMX_EXT = new Set(["dmx"]);
const IMAGE_EXT = new Set(["jpg", "jpeg", "png", "bmp", "gif", "webp", "tiff"]);
const MODEL_EXT = new Set(["obj", "fbx", "stl", "gltf", "glb"]);
const DOCUMENT_EXT = new Set(["txt", "json", "xml", "cfg", "log", "md", "csv", "ini"]);
const CHILDFLOW_EXT = new Set(["childflow"]);

export function detectMediaCategory(fileName: string): MediaCategory {
  const dot = fileName.lastIndexOf(".");
  const ext = dot >= 0 ? fileName.slice(dot + 1).toLowerCase() : "";
  if (VIDEO_EXT.has(ext)) return "video";
  if (AUDIO_EXT.has(ext)) return "audio";
  if (DMX_EXT.has(ext)) return "dmx";
  if (IMAGE_EXT.has(ext)) return "image";
  if (MODEL_EXT.has(ext)) return "model";
  if (CHILDFLOW_EXT.has(ext)) return "childflow";
  if (DOCUMENT_EXT.has(ext)) return "document";
  return "unknown";
}

export function groupMediaFiles(files: FlowMediaFile[]): MediaFileGroup[] {
  const buckets = new Map<MediaCategory, FlowMediaFile[]>();
  for (const file of files) {
    const category = detectMediaCategory(file.name);
    const list = buckets.get(category);
    if (list) list.push(file);
    else buckets.set(category, [file]);
  }

  return MEDIA_CATEGORY_ORDER.flatMap((category) => {
    const items = buckets.get(category);
    if (!items?.length) return [];
    return [{ category, label: MEDIA_CATEGORY_LABELS[category], items }];
  });
}

export async function fetchMediaFiles(): Promise<{
  ok: boolean;
  items: FlowMediaFile[];
  error?: string;
}> {
  try {
    const res = await fetch("/api/media/files");
    const data = (await res.json()) as {
      ok?: boolean;
      items?: FlowMediaFile[];
      error?: string;
    };
    if (data.ok === false) {
      return { ok: false, items: [], error: data.error || "加载失败" };
    }
    return { ok: true, items: Array.isArray(data.items) ? data.items : [] };
  } catch {
    return { ok: false, items: [], error: "无法连接 Flow 媒体服务" };
  }
}

export async function downloadMediaFile(fileName: string): Promise<boolean> {
  try {
    const params = new URLSearchParams({ file: fileName });
    const res = await fetch(`/api/media/download?${params.toString()}`);
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

export function formatMediaFileSize(bytes: number): string {
  if (!Number.isFinite(bytes) || bytes <= 0) return "0 B";
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

export function formatMediaFileTime(iso?: string): string {
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
