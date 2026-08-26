import { marked, type MarkedOptions } from "marked";
import { filterXSS, getDefaultWhiteList, type IWhiteList } from "xss";

let whiteListNormal: IWhiteList | undefined;

/** Port of HA `resources/markdown-worker` renderMarkdown (main-thread version). */
export function renderMarkdown(
  content: string,
  markedOptions: MarkedOptions = {},
  options: { allowSvg?: boolean; allowDataUrl?: boolean } = {},
): string {
  if (!whiteListNormal) {
    whiteListNormal = {
      ...getDefaultWhiteList(),
      table: [...(getDefaultWhiteList().table ?? []), "role"],
      input: ["type", "disabled", "checked"],
      "ha-icon": ["icon"],
      "ha-svg-icon": ["path"],
      "ha-alert": ["alert-type", "title"],
    };
  }

  const whiteList: IWhiteList = { ...whiteListNormal };
  if (options.allowDataUrl && whiteList.a) {
    whiteList.a = [...whiteList.a, "download"];
  }

  marked.setOptions({
    gfm: true,
    breaks: Boolean(markedOptions.breaks),
    ...markedOptions,
  });

  marked.use({
    renderer: {
      table(...args) {
        const defaultRenderer = new marked.Renderer();
        return `<div>${defaultRenderer.table.apply(this, args)}</div>`;
      },
    },
  });

  const rawHtml = marked.parse(content, { async: false }) as string;

  return filterXSS(rawHtml, {
    whiteList,
    onTagAttr: (tag: string, name: string, value: string): string | undefined => {
      if (tag === "input") {
        if (
          (name === "type" && value === "checkbox") ||
          name === "checked" ||
          name === "disabled"
        ) {
          return undefined;
        }
        return "";
      }
      if (
        options.allowDataUrl &&
        tag === "a" &&
        name === "href" &&
        value.startsWith("data:")
      ) {
        return `href="${value}"`;
      }
      return undefined;
    },
  });
}
