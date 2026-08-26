import { historyBuffer, type HistoryPoint } from "./history-buffer";

function steppedSeries(
  start: number,
  end: number,
  steps: [progress: number, value: number][],
): HistoryPoint[] {
  const span = end - start;
  return steps.map(([progress, value]) => [start + progress * span, value]);
}

/** Seed 24h of stepped samples so history-graph has something to draw offline. */
export function seedDemoHistory(now = Date.now()): void {
  const start = now - 24 * 60 * 60 * 1000;

  historyBuffer.replace(
    "/demo/battery_input",
    steppedSeries(start, now, [
      [0, 2.15],
      [0.07, 2.45],
      [0.14, 2.85],
      [0.2, 3.2],
      [0.28, 2.95],
      [0.36, 3.45],
      [0.44, 3.85],
      [0.52, 4.25],
      [0.58, 4.05],
      [0.66, 4.55],
      [0.74, 4.95],
      [0.8, 4.55],
      [0.86, 3.85],
      [0.93, 4.15],
      [1, 4],
    ]),
  );

  historyBuffer.replace(
    "/demo/power",
    steppedSeries(start, now, [
      [0, 620],
      [0.12, 710],
      [0.25, 540],
      [0.4, 880],
      [0.55, 760],
      [0.7, 910],
      [0.85, 798],
      [1, 798],
    ]),
  );
}
