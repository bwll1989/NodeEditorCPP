import { LitElement, css, html, nothing } from "lit";
import { customElement, property } from "lit/decorators.js";
import "./ha-markdown-element";

/** Port of HA `ha-markdown` */
@customElement("ha-markdown")
export class HaMarkdown extends LitElement {
  @property() public content?: string;

  @property({ attribute: "allow-svg", type: Boolean }) public allowSvg = false;

  @property({ type: Boolean }) public breaks = false;

  protected render() {
    if (!this.content) return nothing;

    return html`
      <ha-markdown-element
        .content=${this.content}
        .allowSvg=${this.allowSvg}
        .breaks=${this.breaks}
      ></ha-markdown-element>
    `;
  }

  static styles = css`
    :host {
      display: block;
    }
    ha-markdown-element {
      -ms-user-select: text;
      -webkit-user-select: text;
      -moz-user-select: text;
      user-select: text;
    }
    a {
      color: var(--markdown-link-color, var(--primary-color));
    }
    img {
      max-width: 100%;
    }
    code,
    pre {
      background-color: var(--markdown-code-background-color, rgba(0, 0, 0, 0.06));
      border-radius: var(--ha-border-radius-sm, 4px);
      color: var(--markdown-code-text-color, inherit);
    }
    code {
      font-size: var(--ha-font-size-s, 12px);
      padding: 0.2em 0.4em;
    }
    pre {
      padding: var(--ha-space-4, 16px);
      overflow: auto;
      line-height: var(--ha-line-height-condensed, 1.25);
      font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
    }
    pre code {
      padding: 0;
      background: transparent;
    }
    h1,
    h2,
    h3,
    h4,
    h5,
    h6 {
      line-height: initial;
      margin: 0.6em 0 0.35em;
    }
    h2 {
      font-size: var(--ha-font-size-xl, 22px);
      font-weight: var(--ha-font-weight-bold, 700);
    }
    p {
      margin: 0.5em 0;
    }
    hr {
      border: none;
      border-top: 1px solid var(--divider-color);
      margin: var(--ha-space-4, 16px) 0;
    }
    table {
      border-collapse: collapse;
      width: 100%;
      margin: 0.5em 0;
    }
    td,
    th {
      border: 1px solid var(--divider-color);
      padding: 0.25em 0.5em;
      text-align: start;
    }
    blockquote {
      border-left: 4px solid var(--divider-color);
      margin-inline: 0;
      padding-inline: 1em;
      color: var(--secondary-text-color);
    }
    ul,
    ol {
      padding-inline-start: 1.5em;
      margin: 0.5em 0;
    }
    li:has(input[type="checkbox"]) {
      list-style: none;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-markdown": HaMarkdown;
  }
}
