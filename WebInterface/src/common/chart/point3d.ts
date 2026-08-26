export type Point3 = [number, number, number];

export function numOr(value: unknown, fallback = 0): number {
  if (value === undefined || value === null || value === "") return fallback;
  const n = Number(value);
  return Number.isFinite(n) ? n : fallback;
}

export function toPoint(value: unknown): Point3 | null {
  if (Array.isArray(value) && value.length >= 1) {
    const x = numOr(value[0]);
    const y = numOr(value[1]);
    const z = numOr(value[2]);
    if (
      value
        .slice(0, 3)
        .some((part) => part !== undefined && part !== null && part !== "" && Number.isFinite(Number(part)))
    ) {
      return [x, y, z];
    }
    return null;
  }

  if (value && typeof value === "object") {
    const record = value as Record<string, unknown>;
    const packed = record.default ?? record.values;
    if (Array.isArray(packed) && packed.length >= 1) {
      return toPoint(packed);
    }
    const hasAny =
      record.x !== undefined ||
      record.X !== undefined ||
      record.y !== undefined ||
      record.Y !== undefined ||
      record.z !== undefined ||
      record.Z !== undefined;
    if (!hasAny) return null;
    return [numOr(record.x ?? record.X), numOr(record.y ?? record.Y), numOr(record.z ?? record.Z)];
  }

  return null;
}

export function coercePoints(value: unknown): Point3[] {
  if (Array.isArray(value)) {
    return value.map(toPoint).filter((point): point is Point3 => !!point);
  }
  if (typeof value === "string") {
    const text = value.trim();
    if (!text) return [];
    try {
      return coercePoints(JSON.parse(text));
    } catch {
      return [];
    }
  }
  return [];
}

export function trimPoints(points: Point3[], maxPoints: number): Point3[] {
  const limit = Number.isFinite(maxPoints) && maxPoints > 0 ? Math.floor(maxPoints) : 5000;
  return points.length > limit ? points.slice(-limit) : points;
}

/** Apply WebSocket / entity payload to an existing point buffer. */
export function applyIncoming(points: Point3[], raw: unknown, maxPoints: number): Point3[] {
  if (raw === undefined || raw === null) return points;

  let payload: unknown = raw;
  if (typeof raw === "string") {
    const text = raw.trim();
    if (!text) return points;
    if (text.toLowerCase() === "clear") return [];
    try {
      payload = JSON.parse(text);
    } catch {
      const parts = text.split(/[,;\s]+/).filter((token) => token.length > 0);
      const one = toPoint(parts);
      return one ? trimPoints([...points, one], maxPoints) : points;
    }
  }

  if (payload === "clear") return [];

  if (Array.isArray(payload)) {
    if (
      payload.length >= 1 &&
      payload.length <= 4 &&
      payload.every((item) => typeof item === "number" || typeof item === "string")
    ) {
      const one = toPoint(payload);
      return one ? trimPoints([...points, one], maxPoints) : points;
    }
    return trimPoints(coercePoints(payload), maxPoints);
  }

  if (payload && typeof payload === "object") {
    const record = payload as Record<string, unknown>;
    if (record.clear === true) return [];
    if (Array.isArray(record.points)) {
      const next = coercePoints(record.points);
      if (record.append === true || record.mode === "append") {
        return trimPoints([...points, ...next], maxPoints);
      }
      return trimPoints(next, maxPoints);
    }
    if (record.value !== undefined && record.x === undefined && record.X === undefined) {
      return applyIncoming(points, record.value, maxPoints);
    }
    const one = toPoint(record);
    if (one) return trimPoints([...points, one], maxPoints);
  }

  return points;
}

export function demoSpiralPoints(count = 120): Point3[] {
  const points: Point3[] = [];
  for (let i = 0; i < count; i += 1) {
    const t = i / 12;
    points.push([Math.cos(t) * 6, Math.sin(t) * 6, t * 0.8]);
  }
  return points;
}

export function demoScatterPoints(count = 80): Point3[] {
  const points: Point3[] = [];
  for (let i = 0; i < count; i += 1) {
    points.push([
      (Math.random() - 0.5) * 12,
      (Math.random() - 0.5) * 12,
      (Math.random() - 0.5) * 12,
    ]);
  }
  return points;
}
