import { customElement } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../../../types";
import { chart3dCardStyles, HuiChart3dCardBase } from "./chart-3d-card-shared";

@customElement("hui-scatter3d-card")
export class HuiScatter3dCard extends HuiChart3dCardBase {
  readonly chartMode = "scatter" as const;

  readonly stubEntity = "/demo/scatter3d";

  readonly stubName = "3D Scatter";

  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "scatter3d",
      entity: "/demo/scatter3d",
      name: "3D Scatter",
      max_points: 5000,
      symbol_size: 8,
      show_axes: true,
      auto_rotate: false,
    };
  }

  protected render() {
    return this.renderChart3d();
  }

  static styles = [chart3dCardStyles];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-scatter3d-card": HuiScatter3dCard;
  }
}
