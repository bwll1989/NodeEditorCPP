import { customElement } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../../../types";
import { chart2dCardStyles, HuiChart2dCardBase } from "./chart-2d-card-shared";

@customElement("hui-line2d-card")
export class HuiLine2dCard extends HuiChart2dCardBase {
  readonly chartMode = "line" as const;

  readonly stubEntity = "/demo/line2d";

  readonly stubName = "2D 折线";

  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "line2d",
      entity: "/demo/line2d",
      name: "2D 折线",
      max_points: 5000,
      line_width: 2,
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
    "hui-line2d-card": HuiLine2dCard;
  }
}
