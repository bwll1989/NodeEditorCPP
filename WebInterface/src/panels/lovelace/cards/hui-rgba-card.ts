import { customElement } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../../../types";
import { HuiColorCardBase } from "./color-card-shared";

/** RGBA color card — binds `/rgba` style [r,g,b,a] 0–1 vectors. */
@customElement("hui-rgba-card")
export class HuiRgbaCard extends HuiColorCardBase {
  readonly colorMode = "rgba" as const;

  readonly stubEntity = "/demo/rgba";

  readonly stubName = "RGBA";

  readonly stubIcon = "mdi:palette";

  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "rgba",
      entity: "/demo/rgba",
      name: "RGBA",
      icon: "mdi:palette",
    };
  }
}

/** @deprecated alias kept for existing layouts with type: "color" */
@customElement("hui-color-card")
export class HuiColorCard extends HuiRgbaCard {
  public static getStubConfig(): LovelaceCardConfig {
    return { ...HuiRgbaCard.getStubConfig(), type: "color" };
  }
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-rgba-card": HuiRgbaCard;
    "hui-color-card": HuiColorCard;
  }
}
