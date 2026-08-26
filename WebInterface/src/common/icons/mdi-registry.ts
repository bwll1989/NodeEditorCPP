import {
  mdiAirConditioner,
  mdiAlertCircle,
  mdiBell,
  mdiBrightness6,
  mdiCoffee,
  mdiCog,
  mdiContentCopy,
  mdiContentCut,
  mdiContentPaste,
  mdiCursorMove,
  mdiDelete,
  mdiDishwasher,
  mdiDotsVertical,
  mdiEye,
  mdiFlash,
  mdiFloorPlan,
  mdiFloorLamp,
  mdiFormatTitle,
  mdiFridge,
  mdiGauge,
  mdiGestureTapButton,
  mdiHandWave,
  mdiHelpCircle,
  mdiHome,
  mdiLightbulb,
  mdiLightningBolt,
  mdiMagnify,
  mdiMenu,
  mdiMenuOpen,
  mdiMoleculeCo2,
  mdiCastAudio,
  mdiClockOutline,
  mdiPlay,
  mdiStop,
  mdiPencil,
  mdiPlusCircleMultipleOutline,
  mdiPalette,
  mdiAxisArrow,
  mdiLinkVariant,
  mdiHomeThermometer,
  mdiRedo,
  mdiSilverwareForkKnife,
  mdiSineWave,
  mdiSofa,
  mdiSpotlightBeam,
  mdiText,
  mdiThermometer,
  mdiToggleSwitch,
  mdiTuneVertical,
  mdiUndo,
  mdiViewDashboard,
  mdiViewGrid,
  mdiVolumeHigh,
  mdiVolumeOff,
  mdiWaterPercent,
  mdiWindowShutter,
} from "@mdi/js";

/** Icons used on dashboard — no full library needed for initial render */
const STATIC_PATHS: Record<string, string> = {
  "mdi:home": mdiHome,
  "mdi:hand-wave": mdiHandWave,
  "mdi:sofa": mdiSofa,
  "mdi:thermometer": mdiThermometer,
  "mdi:water-percent": mdiWaterPercent,
  "mdi:floor-lamp": mdiFloorLamp,
  "mdi:spotlight-beam": mdiSpotlightBeam,
  "mdi:window-shutter": mdiWindowShutter,
  "mdi:air-conditioner": mdiAirConditioner,
  "mdi:silverware-fork-knife": mdiSilverwareForkKnife,
  "mdi:coffee": mdiCoffee,
  "mdi:fridge": mdiFridge,
  "mdi:dishwasher": mdiDishwasher,
  "mdi:lightning-bolt": mdiLightningBolt,
  "mdi:flash": mdiFlash,
  "mdi:floor-plan": mdiFloorPlan,
  "mdi:sine-wave": mdiSineWave,
  "mdi:molecule-co2": mdiMoleculeCo2,
  "mdi:view-dashboard": mdiViewDashboard,
  "mdi:cog": mdiCog,
  "mdi:menu": mdiMenu,
  "mdi:menu-open": mdiMenuOpen,
  "mdi:bell": mdiBell,
  "mdi:cast-audio": mdiCastAudio,
  "mdi:clock-outline": mdiClockOutline,
  "mdi:home-thermometer": mdiHomeThermometer,
  "mdi:palette": mdiPalette,
  "mdi:axis-arrow": mdiAxisArrow,
  "mdi:link-variant": mdiLinkVariant,
  "mdi:play": mdiPlay,
  "mdi:stop": mdiStop,
  "mdi:pencil": mdiPencil,
  "mdi:undo": mdiUndo,
  "mdi:redo": mdiRedo,
  "mdi:lightbulb": mdiLightbulb,
  "mdi:help-circle": mdiHelpCircle,
  "mdi:eye": mdiEye,
  "mdi:alert-circle": mdiAlertCircle,
  "mdi:toggle-switch": mdiToggleSwitch,
  "mdi:gesture-tap-button": mdiGestureTapButton,
  "mdi:brightness-6": mdiBrightness6,
  "mdi:format-title": mdiFormatTitle,
  "mdi:view-grid": mdiViewGrid,
  "mdi:gauge": mdiGauge,
  "mdi:tune-vertical": mdiTuneVertical,
  "mdi:volume-high": mdiVolumeHigh,
  "mdi:volume-off": mdiVolumeOff,
  "mdi:text": mdiText,
  "mdi:dots-vertical": mdiDotsVertical,
  "mdi:content-copy": mdiContentCopy,
  "mdi:content-cut": mdiContentCut,
  "mdi:content-paste": mdiContentPaste,
  "mdi:plus-circle-multiple-outline": mdiPlusCircleMultipleOutline,
  "mdi:delete": mdiDelete,
  "mdi:cursor-move": mdiCursorMove,
  "mdi:magnify": mdiMagnify,
};

/** Convert mdi:kebab-name to @mdi/js export key */
export function mdiNameToExportKey(name: string): string {
  const slug = name.replace(/^mdi:/, "");
  if (!slug) return "";
  return (
    "mdi" +
    slug
      .split("-")
      .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
      .join("")
  );
}

export function exportKeyToMdiName(key: string): string {
  let name = key.replace(/^mdi/, "");
  name = name.replace(/([a-z])([A-Z0-9])/g, "$1-$2");
  name = name.replace(/([0-9])([A-Z])/g, "$1-$2");
  return `mdi:${name.toLowerCase()}`;
}

let pathByName: Map<string, string> | null = null;
let allIconNames: string[] | null = null;
let loadPromise: Promise<void> | null = null;
const loadListeners = new Set<() => void>();

export function onMdiRegistryLoaded(listener: () => void): () => void {
  if (pathByName) {
    listener();
    return () => undefined;
  }
  loadListeners.add(listener);
  return () => loadListeners.delete(listener);
}

export async function ensureMdiRegistry(): Promise<void> {
  if (pathByName) return;
  if (!loadPromise) {
    loadPromise = import("@mdi/js").then((mdiIcons) => {
      pathByName = new Map(Object.entries(STATIC_PATHS));
      const names: string[] = [];
      for (const [key, path] of Object.entries(mdiIcons)) {
        if (!key.startsWith("mdi") || typeof path !== "string") continue;
        const name = exportKeyToMdiName(key);
        pathByName.set(name, path);
        names.push(name);
      }
      allIconNames = names.sort();
      for (const listener of loadListeners) listener();
      loadListeners.clear();
    });
  }
  await loadPromise;
}

export function resolveIconPath(icon: string): string {
  if (!icon) return mdiHelpCircle;
  if (pathByName) {
    return pathByName.get(icon) ?? mdiHelpCircle;
  }
  void ensureMdiRegistry();
  return STATIC_PATHS[icon] ?? mdiHelpCircle;
}

export function searchMdiIcons(query: string, limit = 80): string[] {
  const q = query.trim().toLowerCase();
  if (!allIconNames) {
    const staticNames = Object.keys(STATIC_PATHS);
    if (!q) return staticNames.slice(0, limit);
    return staticNames.filter((n) => n.includes(q)).slice(0, limit);
  }
  if (!q) return allIconNames.slice(0, limit);
  const results: string[] = [];
  for (const name of allIconNames) {
    if (name.includes(q)) {
      results.push(name);
      if (results.length >= limit) break;
    }
  }
  return results;
}
