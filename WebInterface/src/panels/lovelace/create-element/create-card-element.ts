import { fireEvent } from "../../../common/dom/fire_event";
import type { LovelaceCard, LovelaceCardConfig } from "../../../types";
import { ensureCardLoaded } from "../../../registry/card-registry";

const TIMEOUT = 2000;

function createErrorCard(message: string, config: LovelaceCardConfig): LovelaceCard {
  const el = document.createElement("hui-error-card") as LovelaceCard;
  el.setConfig({ type: "error", message, origConfig: config } as LovelaceCardConfig);
  return el;
}

export async function createCardElement(config: LovelaceCardConfig): Promise<LovelaceCard> {
  let normalized = config;
  if (config.type === "button") {
    normalized = { ...config, type: "trigger" };
  } else if (config.type === "color") {
    normalized = { ...config, type: "rgba" };
  }
  try {
    const ctor = await ensureCardLoaded(normalized.type);
    if (!ctor) {
      return createErrorCard(`Unknown card type: ${normalized.type}`, normalized);
    }
    const el = new ctor();
    el.setConfig(normalized);
    return el;
  } catch (err) {
    const message = err instanceof Error ? err.message : String(err);
    return createErrorCard(message, config);
  }
}

export function lazyUpgradeCard(
  tag: string,
  config: LovelaceCardConfig,
): LovelaceCard {
  const element = document.createElement(tag) as LovelaceCard;
  ensureCardLoaded(config.type).then((ctor: import("../../../types").LovelaceCardConstructor | undefined) => {
    if (!ctor) return;
    try {
      element.setConfig(config);
      fireEvent(element, "ll-upgrade");
    } catch {
      fireEvent(element, "ll-rebuild");
    }
  });
  return element;
}

export { TIMEOUT };
