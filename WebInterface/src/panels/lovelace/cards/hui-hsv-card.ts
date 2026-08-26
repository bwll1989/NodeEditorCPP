import { customElement } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../../../types";
import { HuiColorCardBase } from "./color-card-shared";

/** HSV color card — binds `/hsv` style [h,s,v] 0–1 vectors (Flow ColorNode). */
@customElement("hui-hsv-card")
export class HuiHsvCard extends HuiColorCardBase {
  readonly colorMode = "hsv" as const;

  readonly stubEntity = "/demo/hsv";

  readonly stubName = "HSV";

  readonly stubIcon = "mdi:palette-swatch";

  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "hsv",
      entity: "/demo/hsv",
      name: "HSV",
      icon: "mdi:palette-swatch",
    };
  }
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-hsv-card": HuiHsvCard;
  }
}
