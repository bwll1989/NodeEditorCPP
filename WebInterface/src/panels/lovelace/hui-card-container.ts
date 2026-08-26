import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { consume } from "@lit/context";
import type { Flow, LovelaceCard, LovelaceCardConfig } from "../../types";
import { flowContext } from "../../context/flow-context";
import { collectCardEntityAddresses } from "../../data/flow";
import { createCardElement } from "./create-element/create-card-element";
import {
  cardGridSizeToCssVars,
  computeCardGridSize,
} from "./common/compute-card-grid-size";
import { getMergedGridOptionsFromCard } from "./common/get-card-grid-options";
import "./components/hui-card-edit-mode";

@customElement("hui-card-container")
export class HuiCardContainer extends LitElement {
  @consume({ context: flowContext, subscribe: true })
  @property({ attribute: false })
  public flow?: Flow;

  @property({ attribute: false }) public config?: LovelaceCardConfig;

  @property({ type: Number }) public sectionIndex = 0;

  @property({ type: Number }) public cardIndex = 0;

  /** SortableJS payload — [viewIndex, sectionIndex, cardIndex] */
  @property({ attribute: false }) public sortableData?: unknown;

  @property({ type: Boolean }) public editMode = false;

  @property({ type: Boolean }) public preview = false;

  @state() private _card?: LovelaceCard;

  @state() private _statesRevision = 0;

  private _configKey = "";

  private _entityUnsubs: Array<() => void> = [];

  private _refreshCard(): void {
    const card = this._card as (LovelaceCard & { requestUpdate?: () => void }) | undefined;
    card?.requestUpdate?.();
    this.requestUpdate();
  }

  connectedCallback(): void {
    super.connectedCallback();
  }

  disconnectedCallback(): void {
    this._clearEntitySubscriptions();
    super.disconnectedCallback();
  }

  private _clearEntitySubscriptions(): void {
    this._entityUnsubs.forEach((unsub) => unsub());
    this._entityUnsubs = [];
  }

  private _syncEntitySubscriptions(): void {
    this._clearEntitySubscriptions();
    const flow = this.flow;
    if (!flow || !this._card) return;

    const addresses = collectCardEntityAddresses(this.config);
    if (addresses.length === 0) {
      this._entityUnsubs.push(
        flow.subscribeStates(() => {
          this._statesRevision = flow.statesRevision;
          this._refreshCard();
        }),
      );
      return;
    }

    addresses.forEach((address) => {
      this._entityUnsubs.push(
        flow.subscribeEntity(address, () => {
          this._statesRevision = flow.statesRevision;
          this._refreshCard();
        }),
      );
    });
  }

  private _lastType = "";

  protected async updated(changed: PropertyValues): Promise<void> {
    if (changed.has("config") && this.config) {
      const key = JSON.stringify(this.config);
      if (key !== this._configKey) {
        this._configKey = key;
        if (this._card && this._lastType === this.config.type) {
          this._card.setConfig(this.config);
        } else {
          this._lastType = this.config.type;
          const card = await createCardElement(this.config);
          card.flow = this.flow;
          card.preview = this.preview;
          this._card = card;
        }
      }
    }
    if (this._card && (changed.has("flow") || changed.has("config") || changed.has("preview"))) {
      this._card.flow = this.flow;
      this._card.preview = this.preview;
    }
    if (changed.has("flow") || changed.has("config")) {
      this._syncEntitySubscriptions();
    }
    if (this._card) {
      this._applyGridSize();
    }
  }

  private _applyGridSize(): void {
    const merged = getMergedGridOptionsFromCard(this._card, this.config);
    const gridSize = computeCardGridSize(merged);
    const { columnSize, rowSize, fullWidth, autoHeight } =
      cardGridSizeToCssVars(gridSize);

    this.style.setProperty("--column-size", String(columnSize));
    this.style.setProperty("--row-size", String(rowSize));
    this.classList.toggle("full-width", fullWidth);
    this.classList.toggle("fit-rows", !autoHeight);
    this.toggleAttribute("auto-height", autoHeight);
  }

  protected render() {
    if (!this._card) return nothing;

    const isHeading = this.config?.type === "heading";
    const isChartInteractive =
      this.config?.type === "line3d" ||
      this.config?.type === "scatter3d" ||
      this.config?.type === "line2d" ||
      this.config?.type === "scatter2d" ||
      this.config?.type === "bar";
    // Bind Lit to remote state changes from Flow / WebSocket.
    void this._statesRevision;

    const card = html`
      <hui-card-edit-mode
        .inactive=${!this.editMode}
        .sectionIndex=${this.sectionIndex}
        .cardIndex=${this.cardIndex}
        .noEdit=${false}
        .compact=${isHeading || isChartInteractive}
        .hiddenOverlay=${isHeading}
      >
        ${this._card}
      </hui-card-edit-mode>
    `;

    return html`${card}`;
  }

  static styles = css`
    :host {
      display: block;
      position: relative;
      grid-row: span var(--row-size, 1);
      grid-column: span min(var(--column-size, 1), var(--grid-column-count, 12));
      min-width: 0;
      border-radius: var(--ha-card-border-radius, 12px);
    }
    :host(.full-width) {
      grid-column: 1 / -1;
    }
    :host(.fit-rows) {
      height: calc(
        (var(--row-size, 1) * (var(--row-height, 56px) + var(--row-gap, 8px))) -
          var(--row-gap, 8px)
      );
    }
    :host([auto-height]) {
      height: auto;
      min-height: auto;
    }
    :host(.dragging) {
      opacity: 0.5;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-card-container": HuiCardContainer;
  }
}
