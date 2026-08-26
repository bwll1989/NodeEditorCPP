export type ClockSize = "small" | "medium" | "large";
export type ClockStyle = "digital" | "analog";
export type ClockTicks = "none" | "quarter" | "hour" | "minute";

export function resolveTimeZone(timeZone?: string): string {
  return timeZone?.trim() || Intl.DateTimeFormat().resolvedOptions().timeZone;
}

export function resolveLocale(): string {
  return navigator.language || "zh-CN";
}

export function useAmPm(timeFormat?: string): boolean {
  if (timeFormat === "12") return true;
  if (timeFormat === "24") return false;
  return (
    new Intl.DateTimeFormat(resolveLocale(), { hour: "numeric" }).resolvedOptions()
      .hour12 ?? false
  );
}

export function shouldShowDate(size?: ClockSize): boolean {
  return size === "medium" || size === "large";
}

export function formatClockDate(
  date: Date,
  size: ClockSize = "small",
  timeZone?: string,
  locale = resolveLocale(),
): string {
  if (!shouldShowDate(size)) return "";
  const options: Intl.DateTimeFormatOptions =
    size === "large"
      ? { weekday: "long", day: "numeric", month: "long", timeZone: resolveTimeZone(timeZone) }
      : { weekday: "short", day: "numeric", month: "short", timeZone: resolveTimeZone(timeZone) };
  return new Intl.DateTimeFormat(locale, options).format(date);
}

export interface ZonedTimeParts {
  hour: number;
  minute: number;
  second: number;
  hour12: number;
  dayPeriod?: string;
}

export function getZonedTimeParts(
  date: Date,
  timeZone?: string,
  hour12 = false,
): ZonedTimeParts {
  const formatter = new Intl.DateTimeFormat("en-US", {
    hour: "numeric",
    minute: "numeric",
    second: "numeric",
    hour12,
    timeZone: resolveTimeZone(timeZone),
  });
  const parts = formatter.formatToParts(date);
  const read = (type: Intl.DateTimeFormatPartTypes) =>
    Number(parts.find((part) => part.type === type)?.value ?? 0);
  const hour = read("hour");
  const minute = read("minute");
  const second = read("second");
  const dayPeriod = parts.find((part) => part.type === "dayPeriod")?.value;
  const hour12Value = hour12 ? hour : hour % 12 || 12;
  return {
    hour,
    minute,
    second,
    hour12: hour12Value,
    dayPeriod,
  };
}
