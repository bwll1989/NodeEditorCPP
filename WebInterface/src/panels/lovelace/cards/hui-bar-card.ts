import { customElement } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../../../types";
import { chart2dCardStyles, HuiChart2dCardBase } from "./chart-2d-card-shared";

@customElement("hui-bar-card")
export class HuiBarCard extends HuiChart2dCardBase {
  readonly chartMode = "bar" as const;

  readonly stubEntity = "";

  readonly stubName = "柱状图";

  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "bar",
      name: "柱状图",
      entities: [
        { entity: "/demo/power", name: "Power" },
        { entity: "/demo/voltage", name: "Voltage" },
        { entity: "/demo/co2", name: "CO₂" },
      ],
      show_axes: true,
      show_grid: true,
    };
  }

  protected render() {
    return this.renderChart2d();
  }

  static styles = [chart2dCardStyles];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-bar-card": HuiBarCard;
  }
}
