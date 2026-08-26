import { css, html, LitElement } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { onMdiRegistryLoaded, resolveIconPath, ensureMdiRegistry } from "../common/icons/mdi-registry";

@customElement("ha-svg-icon")
export class HaSvgIcon extends LitElement {
  @property({ attribute: false }) public path = "";

  protected render() {
    return html`
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d=${this.path}></path>
      </svg>
    `;
  }

  static styles = css`
    :host {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size, 24px);
      height: var(--mdc-icon-size, 24px);
      line-height: 0;
      flex-shrink: 0;
      vertical-align: middle;
    }
    svg {
      display: block;
      width: 100%;
      height: 100%;
      fill: currentColor;
    }
  `;
}

@customElement("ha-icon")
export class HaIcon extends LitElement {
  @property() public icon = "";

  @state() private _revision = 0;

  private _unload?: () => void;

  connectedCallback(): void {
    super.connectedCallback();
    void ensureMdiRegistry();
    this._unload = onMdiRegistryLoaded(() => {
      this._revision++;
    });
  }

  disconnectedCallback(): void {
    this._unload?.();
    super.disconnectedCallback();
  }

  protected render() {
    void this._revision;
    const path = resolveIconPath(this.icon);
    return html`<ha-svg-icon .path=${path}></ha-svg-icon>`;
  }

  static styles = css`
    :host {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: var(--mdc-icon-size, 24px);
      height: var(--mdc-icon-size, 24px);
      line-height: 0;
      flex-shrink: 0;
      color: inherit;
    }
  `;
}

export { resolveIconPath } from "../common/icons/mdi-registry";

declare global {
  interface HTMLElementTagNameMap {
    "ha-icon": HaIcon;
    "ha-svg-icon": HaSvgIcon;
  }
}
