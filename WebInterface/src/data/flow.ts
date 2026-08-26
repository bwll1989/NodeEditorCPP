import type { Flow, FlowValue } from "../types";
import type { FlowStore } from "./flow-store";
import { applyDemoService } from "./demo-service";
import type { FlowWebSocketConnection } from "./connection";
import {
  formatHsvaDisplay,
  formatRgbaDisplay,
  parseHsvaValue,
  parseRgbaValue,
} from "../common/entity/color";

export function createFlow(
  store: FlowStore,
  connection: FlowWebSocketConnection,
): Flow {
  let statesRevision = 0;
  store.subscribeAll(() => {
    statesRevision += 1;
  });

  return {
    get connected() {
      return connection.connected;
    },
    get states() {
      return store.getStatesRecord();
    },
    get statesRevision() {
      return statesRevision;
    },
    connection,
    subscribeStates(callback) {
      return connection.subscribeStates(callback);
    },
    subscribeEntity(address, callback) {
      return store.subscribe(address, callback);
    },
    async callService(address: string, value: FlowValue): Promise<void> {
      connection.sendCommand(address, value);
      if (!connection.connected) {
        applyDemoService(store, address, value);
      }
    },
    formatState(address: string): string {
      const state = store.getState(address);
      if (!state) return "—";
      const v = state.state;
      if (typeof v === "boolean") return v ? "开启" : "关闭";
      if (Array.isArray(v)) {
        if (address.includes("hsv") && v.length === 3) {
          const hsva = parseHsvaValue(v);
          if (hsva) return formatHsvaDisplay(hsva);
        }
        const rgba = parseRgbaValue(v);
        return rgba ? formatRgbaDisplay(rgba) : v.join(", ");
      }
      if (v === null || v === undefined) return "—";
      const unit = state.attributes.unit_of_measurement;
      return unit ? `${v} ${unit}` : String(v);
    },
  };
}

export function collectCardEntityAddresses(config?: {
  entity?: string;
  badges?: { entity?: string }[];
  elements?: { entity?: string }[];
  entities?: { entity?: string }[];
  [key: string]: unknown;
}): string[] {
  if (!config) return [];
  const addresses = new Set<string>();
  if (config.entity) addresses.add(config.entity);
  const muteEntity = config.mute_entity;
  if (typeof muteEntity === "string" && muteEntity) addresses.add(muteEntity);
  const entityX = config.entity_x;
  if (typeof entityX === "string" && entityX) addresses.add(entityX);
  const entityY = config.entity_y;
  if (typeof entityY === "string" && entityY) addresses.add(entityY);
  config.badges?.forEach((badge) => {
    if (badge.entity) addresses.add(badge.entity);
  });
  config.elements?.forEach((element) => {
    if (element.entity) addresses.add(element.entity);
  });
  config.entities?.forEach((item) => {
    if (item.entity) addresses.add(item.entity);
  });
  return [...addresses];
}
