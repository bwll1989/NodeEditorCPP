import type { PropertyValues } from "lit";
import { ReactiveElement } from "lit";
import { customElement, property } from "lit/decorators.js";
import { renderMarkdown } from "../common/markdown/render-markdown";

/** Port of HA `ha-markdown-element` (Light DOM + sanitized HTML). */
@customElement("ha-markdown-element")
export class HaMarkdownElement extends ReactiveElement {
  @property() public content?: string;

  @property({ attribute: "allow-svg", type: Boolean }) public allowSvg = false;

  @property({ type: Boolean }) public breaks = false;

  protected createRenderRoot() {
    return this;
  }

  protected update(changedProps: PropertyValues): void {
    super.update(changedProps);
    if (this.content !== undefined && changedProps.has("content")) {
      this._renderMarkdown();
    } else if (
      changedProps.has("breaks") ||
      changedProps.has("allowSvg")
    ) {
      this._renderMarkdown();
    }
  }

  private _renderMarkdown(): void {
    const html = renderMarkdown(
      String(this.content ?? ""),
      {
        breaks: this.breaks,
        gfm: true,
      },
      {
        allowSvg: this.allowSvg,
      },
    );
    this.innerHTML = html;

    // Open external links in a new tab (HA behavior)
    this.querySelectorAll("a").forEach((anchor) => {
      try {
        if (anchor.host && anchor.host !== document.location.host) {
          anchor.target = "_blank";
          anchor.rel = "noreferrer noopener";
        }
      } catch {
        // ignore invalid URLs
      }
    });
  }
}

declare global {
  interface HTMLElementTagNameMap {
    "ha-markdown-element": HaMarkdownElement;
  }
}
