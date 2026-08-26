import { demoLine2dPoints, demoScatter2dPoints } from "../common/chart/point2d";
import { demoScatterPoints, demoSpiralPoints } from "../common/chart/point3d";
import type { FlowStore } from "./flow-store";
import type { FlowValue } from "../types";

type DemoEntry =
  | FlowValue
  | {
      value: FlowValue;
      attributes?: Record<string, unknown>;
    };

/** Local demo states when backend is offline — mirrors HA sections demo feel */
export function seedDemoStates(store: FlowStore): void {
  const demo: Record<string, DemoEntry> = {
    "/demo/floor_lamp": true,
    "/demo/spotlights": 70,
    "/demo/coffee": false,
    "/demo/fridge": true,
    "/demo/dishwasher": false,
    "/demo/power": 797.86,
    "/demo/voltage": 235.61,
    "/demo/co2": 458,
    "/demo/temperature": {
      value: 10.2,
      attributes: { unit_of_measurement: "°C" },
    },
    "/demo/humidity": {
      value: 56,
      attributes: { unit_of_measurement: "%" },
    },
    "/demo/string": {
      value: "Intro_Loop_v3.mp4",
      attributes: {
        friendly_name: "当前素材",
        icon: "mdi:filmstrip",
      },
    },
    "/demo/brightness": 70,
    "/demo/trigger": false,
    "/demo/switch": false,
    "/demo/climate": {
      value: 23,
      attributes: {
        friendly_name: "Upstairs",
        current_temperature: 21.7,
      },
    },
    "/demo/gain": {
      value: -18,
      attributes: {
        friendly_name: "输入增益",
        unit_of_measurement: "dB",
      },
    },
    "/demo/rgba": {
      value: [0.12, 0.53, 0.9, 1],
      attributes: {
        friendly_name: "RGBA",
        icon: "mdi:palette",
      },
    },
    "/demo/hsv": {
      value: [0.58, 0.87, 0.9],
      attributes: {
        friendly_name: "HSV",
        icon: "mdi:palette-swatch",
      },
    },
    "/demo/faders": {
      value: [0.25, 0.5, 0.75, 0.4],
      attributes: {
        friendly_name: "多维推杆",
        icon: "mdi:tune-vertical-variant",
      },
    },
    "/demo/xy": {
      value: [0.5, 0.5],
      attributes: {
        friendly_name: "XY Pad",
        icon: "mdi:axis-arrow",
      },
    },
    "/demo/media": {
      value: true,
      attributes: { friendly_name: "媒体播放器" },
    },
    "/demo/status_ws": {
      value: true,
      attributes: { friendly_name: "connected", icon: "mdi:lan-connect" },
    },
    "/demo/status_tcp": {
      value: true,
      attributes: { friendly_name: "tcp连接", icon: "mdi:lan-connect" },
    },
    "/demo/status_udp": {
      value: true,
      attributes: { friendly_name: "connected", icon: "mdi:lan-connect" },
    },
    "/demo/status_osc": {
      value: true,
      attributes: { friendly_name: "connected", icon: "mdi:lan-connect" },
    },
    "/demo/kitchen_shutter": {
      value: "open",
      attributes: {
        friendly_name: "Kitchen shutter",
        current_position: 3,
      },
    },
    "/demo/line3d": {
      value: "",
      attributes: { points: demoSpiralPoints(120) },
    },
    "/demo/scatter3d": {
      value: "",
      attributes: { points: demoScatterPoints(80) },
    },
    "/demo/line2d": {
      value: "",
      attributes: { points: demoLine2dPoints(80) },
    },
    "/demo/scatter2d": {
      value: "",
      attributes: { points: demoScatter2dPoints(60) },
    },
  };

  Object.entries(demo).forEach(([address, entry]) => {
    if (typeof entry === "object" && entry !== null && "value" in entry) {
      store.applyUpdate(address, entry.value, entry.attributes ?? {});
    } else {
      store.applyUpdate(address, entry);
    }
  });
}
