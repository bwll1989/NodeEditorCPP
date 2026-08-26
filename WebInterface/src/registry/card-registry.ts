import type { CardRegistryEntry, LovelaceCardConstructor } from "../types";

const registry = new Map<string, CardRegistryEntry>();

export function registerCard(entry: CardRegistryEntry): void {
  registry.set(entry.type, entry);
}

export function getCardEntry(type: string): CardRegistryEntry | undefined {
  return registry.get(type);
}

export function listCards(): CardRegistryEntry[] {
  return [...registry.values()];
}

function hasCardConstructor(
  entry: CardRegistryEntry,
): entry is CardRegistryEntry & { constructor: LovelaceCardConstructor } {
  return Object.hasOwn(entry, "constructor") && typeof entry.constructor === "function";
}

export async function ensureCardLoaded(type: string): Promise<LovelaceCardConstructor | undefined> {
  const entry = registry.get(type);
  if (!entry) return undefined;
  if (hasCardConstructor(entry)) return entry.constructor;
  if (entry.loader) {
    await entry.loader();
  }
  return hasCardConstructor(entry) ? entry.constructor : undefined;
}

export function getPickerCards(): CardRegistryEntry[] {
  // Hide legacy aliases from the add-card picker.
  return listCards().filter((c) => c.type !== "error" && c.type !== "color");
}
