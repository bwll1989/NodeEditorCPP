import { customElement } from "lit/decorators.js";
import type { LovelaceCardConfig } from "../../../types";
import { chart3dCardStyles, HuiChart3dCardBase } from "./chart-3d-card-shared";

@customElement("hui-line3d-card")
export class HuiLine3dCard extends HuiChart3dCardBase {
  readonly chartMode = "line" as const;

  readonly stubEntity = "/demo/line3d";

  readonly stubName = "3D Line";

  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "line3d",
      entity: "/demo/line3d",
      name: "3D Line",
      max_points: 5000,
      line_width: 3,
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
    "hui-line3d-card": HuiLine3dCard;
  }
}
