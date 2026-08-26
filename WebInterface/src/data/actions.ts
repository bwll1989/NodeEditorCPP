export type FlowActionAccess = "read" | "write" | "readwrite";

export interface FlowAction {
  entity: string;
  name?: string;
  suggestedName?: string;
  icon?: string;
  access?: FlowActionAccess;
  used?: boolean;
  nodeId?: number;
  nodeName?: string;
  nodeType?: string;
  relative?: string;
  createdAt?: number;
  /** @deprecated 旧数据兼容，新动作库不再使用 */
  id?: string;
}

export function actionDisplayName(action: FlowAction): string {
  const name = action.name?.trim() || action.suggestedName?.trim();
  if (name) {
    return name.replace(/^\d+:\s*/, "");
  }
  const nodeName = action.nodeName?.trim();
  if (nodeName) return nodeName;
  const nodeType = action.nodeType?.trim();
  if (nodeType) return nodeType;
  return action.entity;
}

export function actionAccessLabel(access?: FlowActionAccess): string {
  switch (access) {
    case "read":
      return "只读";
    case "write":
      return "只写";
    default:
      return "读写";
  }
}

export function findActionByEntity(
  actions: FlowAction[],
  entity: string,
): FlowAction | undefined {
  const key = entity.trim();
  if (!key) return undefined;
  return actions.find((action) => action.entity === key);
}

export function findActionsByName(
  actions: FlowAction[],
  query: string,
): FlowAction[] {
  const filter = query.trim().toLowerCase();
  if (!filter) return [...actions];
  return actions.filter((action) =>
    actionDisplayName(action).toLowerCase().includes(filter),
  );
}

export async function createAction(
  entity: string,
  name?: string,
): Promise<FlowAction | undefined> {
  const trimmedEntity = entity.trim();
  if (!trimmedEntity) return undefined;
  const trimmedName = name?.trim() || trimmedEntity;
  try {
    const res = await fetch("/api/actions", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ entity: trimmedEntity, name: trimmedName }),
    });
    if (!res.ok) return undefined;
    const data = (await res.json()) as { ok?: boolean; item?: FlowAction };
    notifyActionsChanged();
    return data.item;
  } catch {
    return undefined;
  }
}

export async function fetchActions(): Promise<FlowAction[]> {
  try {
    const res = await fetch("/api/actions");
    if (!res.ok) return [];
    const data = (await res.json()) as { ok?: boolean; items?: FlowAction[] };
    if (!Array.isArray(data.items)) return [];
    return data.items;
  } catch {
    return [];
  }
}

export async function patchAction(
  entity: string,
  patch: Partial<Pick<FlowAction, "name" | "icon" | "access">>,
): Promise<FlowAction | undefined> {
  try {
    const res = await fetch("/api/actions/patch", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ entity, ...patch }),
    });
    if (!res.ok) return undefined;
    const data = (await res.json()) as { ok?: boolean; item?: FlowAction };
    return data.item;
  } catch {
    return undefined;
  }
}

export async function deleteAction(entity: string): Promise<boolean> {
  try {
    const res = await fetch("/api/actions/remove", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ entity }),
    });
    if (!res.ok) return false;
    const data = (await res.json()) as { ok?: boolean };
    return Boolean(data.ok);
  } catch {
    return false;
  }
}

export function notifyActionsChanged(): void {
  document.dispatchEvent(new CustomEvent("flow-actions-changed"));
}
