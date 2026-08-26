import { css } from "lit";

/** Shared tile card styles (from HA tile-card-style.ts) */
export const tileCardStyle = css`
  ha-card {
    height: 100%;
    transition:
      box-shadow 180ms ease-in-out,
      border-color 180ms ease-in-out;
  }

  ha-card:hover {
    box-shadow:
      var(--ha-card-box-shadow),
      0 2px 6px rgba(0, 0, 0, 0.06);
  }

  ha-tile-icon {
    --tile-icon-color: var(--tile-color);
  }
`;

/** Default inactive/active tile colors on card host (from HA hui-tile-card). */
export const tileCardHostStyle = css`
  :host {
    --tile-color: var(--state-inactive-color);
  }

  ha-card.active {
    --tile-color: var(--state-icon-color);
  }
`;
