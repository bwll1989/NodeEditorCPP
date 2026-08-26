export type Point2 = [number, number];

export function numOr(value: unknown, fallback = 0): number {
  if (value === undefined || value === null || value === "") return fallback;
  const n = Number(value);
  return Number.isFinite(n) ? n : fallback;
}

export function toPoint2(value: unknown): Point2 | null {
  if (Array.isArray(value) && value.length >= 1) {
    const x = numOr(value[0]);
    const y = numOr(value[1]);
    if (
      value
        .slice(0, 2)
        .some((part) => part !== undefined && part !== null && part !== "" && Number.isFinite(Number(part)))
    ) {
      return [x, y];
    }
    return null;
  }

  if (value && typeof value === "object") {
    const record = value as Record<string, unknown>;
    const packed = record.default ?? record.values;
    if (Array.isArray(packed) && packed.length >= 1) {
      return toPoint2(packed);
    }
    const hasAny =
      record.x !== undefined ||
      record.X !== undefined ||
      record.y !== undefined ||
      record.Y !== undefined;
    if (!hasAny) return null;
    return [numOr(record.x ?? record.X), numOr(record.y ?? record.Y)];
  }

  return null;
}

export function coercePoints2(value: unknown): Point2[] {
  if (Array.isArray(value)) {
    return value.map(toPoint2).filter((point): point is Point2 => !!point);
  }
  if (typeof value === "string") {
    const text = value.trim();
    if (!text) return [];
    try {
      return coercePoints2(JSON.parse(text));
    } catch {
      return [];
    }
  }
  return [];
}

export function trimPoints2(points: Point2[], maxPoints: number): Point2[] {
  const limit = Number.isFinite(maxPoints) && maxPoints > 0 ? Math.floor(maxPoints) : 5000;
  return points.length > limit ? points.slice(-limit) : points;
}

/** Apply WebSocket / entity payload to an existing 2D point buffer. */
export function applyIncoming2(points: Point2[], raw: unknown, maxPoints: number): Point2[] {
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
      const one = toPoint2(parts);
      return one ? trimPoints2([...points, one], maxPoints) : points;
    }
  }

  if (payload === "clear") return [];

  if (Array.isArray(payload)) {
    if (
      payload.length >= 1 &&
      payload.length <= 3 &&
      payload.every((item) => typeof item === "number" || typeof item === "string")
    ) {
      const one = toPoint2(payload);
      return one ? trimPoints2([...points, one], maxPoints) : points;
    }
    return trimPoints2(coercePoints2(payload), maxPoints);
  }

  if (payload && typeof payload === "object") {
    const record = payload as Record<string, unknown>;
    if (record.clear === true) return [];
    if (Array.isArray(record.points)) {
      const next = coercePoints2(record.points);
      if (record.append === true || record.mode === "append") {
        return trimPoints2([...points, ...next], maxPoints);
      }
      return trimPoints2(next, maxPoints);
    }
    if (record.value !== undefined && record.x === undefined && record.X === undefined) {
      return applyIncoming2(points, record.value, maxPoints);
    }
    const one = toPoint2(record);
    if (one) return trimPoints2([...points, one], maxPoints);
  }

  return points;
}

export function demoLine2dPoints(count = 80): Point2[] {
  const points: Point2[] = [];
  for (let i = 0; i < count; i += 1) {
    const x = i / 4;
    points.push([x, Math.sin(x / 3) * 5 + Math.cos(x / 7) * 2]);
  }
  return points;
}

export function demoScatter2dPoints(count = 60): Point2[] {
  const points: Point2[] = [];
  for (let i = 0; i < count; i += 1) {
    points.push([(Math.random() - 0.5) * 12, (Math.random() - 0.5) * 12]);
  }
  return points;
}
