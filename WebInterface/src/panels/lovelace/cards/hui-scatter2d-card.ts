import { customElement } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../../../types";
import { chart2dCardStyles, HuiChart2dCardBase } from "./chart-2d-card-shared";

@customElement("hui-scatter2d-card")
export class HuiScatter2dCard extends HuiChart2dCardBase {
  readonly chartMode = "scatter" as const;

  readonly stubEntity = "/demo/scatter2d";

  readonly stubName = "2D 散点";

  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "scatter2d",
      entity: "/demo/scatter2d",
      name: "2D 散点",
      max_points: 5000,
      symbol_size: 8,
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
    "hui-scatter2d-card": HuiScatter2dCard;
  }
}
