import type { FlowValue } from "../types";
import {
  hsvaToFlowValue,
  parseHsvaValue,
  parseRgbaValue,
  rgbaToFlowValue,
} from "../common/entity/color";
import { parseXyValue, xyToFlowValue } from "../common/entity/xy-pad";
import { parseVectorValue, vectorToFlowValue } from "../common/entity/multi-fader";
import type { FlowStore } from "./flow-store";

/** Optimistic demo updates when WebSocket is offline. */
export function applyDemoService(store: FlowStore, address: string, value: FlowValue): void {
  if (!address.startsWith("/demo/")) return;

  const current = store.getState(address);
  if (!current) return;

  if (address.includes("climate") || address.includes("thermostat") || address === "/demo/ac") {
    if (typeof value === "number") {
      if (typeof current.state === "number") {
        store.applyUpdate(address, value, current.attributes);
      } else {
        store.applyUpdate(address, current.state, {
          ...current.attributes,
          temperature: value,
        });
      }
    }
    return;
  }

  if (address.includes("gain") && typeof value === "number") {
    store.applyUpdate(address, value, current.attributes);
    return;
  }

  if (address.includes("hsv")) {
    const hsva = parseHsvaValue(value);
    if (hsva) {
      store.applyUpdate(address, hsvaToFlowValue(hsva), current.attributes);
    }
    return;
  }

  if (address.includes("rgba") || address.includes("color")) {
    const rgba = parseRgbaValue(value);
    if (rgba) {
      store.applyUpdate(address, rgbaToFlowValue(rgba), current.attributes);
    }
    return;
  }

  if (address.includes("fader") || address.includes("vector")) {
    if (Array.isArray(value)) {
      const parsed = parseVectorValue(value, value.length, {
        min: -Infinity,
        max: Infinity,
        step: 0,
      });
      store.applyUpdate(address, vectorToFlowValue(parsed), current.attributes);
    }
    return;
  }

  if (address.includes("xy")) {
    const point = parseXyValue(value);
    if (point) {
      store.applyUpdate(address, xyToFlowValue(point), current.attributes);
    } else if (typeof value === "number") {
      store.applyUpdate(address, value, current.attributes);
    }
    return;
  }

  if (address.includes("shutter") || address.includes("cover")) {
    if (typeof value === "number") {
      const position = Math.round(value);
      const closed = position <= 0;
      store.applyUpdate(address, closed ? "closed" : "open", {
        ...current.attributes,
        current_position: position,
      });
    }
    return;
  }

  // Momentary trigger: pulse to true, then settle back to false.
  if (address.includes("trigger") || address.includes("button")) {
    store.applyUpdate(address, true, current.attributes);
    window.setTimeout(() => {
      const latest = store.getState(address);
      store.applyUpdate(address, false, latest?.attributes ?? current.attributes);
    }, 700);
    return;
  }

  store.applyUpdate(address, value, current.attributes);
}
