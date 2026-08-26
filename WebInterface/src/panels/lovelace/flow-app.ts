import { css, html, LitElement, nothing, type PropertyValues } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { provide } from "@lit/context";
import { mdiCheck, mdiLanConnect, mdiLanDisconnect, mdiPencil, mdiRedo, mdiUndo } from "@mdi/js";
import type { Flow, LovelaceCardConfig, LovelaceConfig, LovelaceSectionConfig, LovelaceViewConfig } from "../../types";
import { flowContext } from "../../context/flow-context";
import {
  type ConfirmDialogParams,
  resolveConfirmationDialog,
  resolveDeleteCardDialog,
  showConfirmationDialog,
  showDeleteCardDialog,
} from "../../dialogs/show-confirm-dialog";
import {
  addCard,
  addSection,
  addView,
  deleteCard,
  deleteSection,
  deleteView,
  duplicateCard,
  duplicateSection,
  getActiveView,
  replaceSection,
  resolveViewIndex,
  updateCard,
  updateSection,
  updateView,
  viewUrlPath,
} from "../../data/layout-mutations";
import { collectEntities, saveLayout } from "../../data/layout";
import { generateDefaultSection, generateDefaultView } from "../../data/section-config";
import {
  isLayoutEditEnabled,
  LAYOUT_EDIT_ENABLED_EVENT,
} from "../../data/layout-edit-mode";
import {
  pushRecentCardType,
  setCardClipboard,
} from "../../data/dashboard-card-clipboard";
import { UndoRedoController } from "../../data/undo-redo-controller";
import { ensureCardLoaded } from "../../registry/card-registry";
import type { FlowPanel } from "../../components/ha-sidebar";
import { listenMediaQuery } from "../../common/dom/media_query";
import "./flow-view";
import "./components/flow-view-tabs";
import "./editor/flow-card-editor";
import "./editor/flow-card-picker";
import "./editor/flow-section-editor";
import "./editor/flow-view-editor";
import "../settings/flow-settings-panel";
import "../../dialogs/flow-confirm-dialog";
import "../../dialogs/flow-delete-card-dialog";
import "../../components/ha-sidebar";
import "../../components/ha-drawer";
import "../../components/ha-menu-button";
import "../../components/ha-icon-button";
import "../../components/ha-icon";
import "../../components/ha-card";

const SIDEBAR_EXPANDED_KEY = "flow_sidebar_expanded";
const NARROW_QUERY = "(max-width: 870px)";

interface CardEditTarget {
  sectionIndex: number;
  cardIndex: number;
}

interface UndoStackItem {
  viewIndex: number;
  config: LovelaceConfig;
}

@customElement("flow-app")
export class FlowApp extends LitElement {
  @provide({ context: flowContext })
  @property({ attribute: false })
  public flow!: Flow;

  @property({ attribute: false }) public lovelace?: LovelaceConfig;

  @state() private _activeView = 0;
  @state() private _panel: FlowPanel = "home";
  @state() private _editMode = false;
  @state() private _pickerOpen = false;
  @state() private _pickerSection?: LovelaceSectionConfig;
  @state() private _cardEditorOpen = false;
  @state() private _cardEditorTarget?: CardEditTarget;
  @state() private _sectionEditorOpen = false;
  @state() private _sectionEditorTarget?: number;
  @state() private _viewEditorOpen = false;
  @state() private _viewEditorIsNew = false;
  @state() private _viewEditorTarget?: number;
  @state() private _confirmOpen = false;
  @state() private _confirmParams?: ConfirmDialogParams;
  @state() private _deleteCardOpen = false;
  @state() private _deleteCardConfig?: LovelaceCardConfig;
  @state() private _layoutEditEnabled = false;
  /** Bumped on every layout mutation so children re-render without toggling edit mode. */
  @state() private _layoutRevision = 0;
  /** HA: viewport ≤ 870px uses modal sidebar + top app bar menu button. */
  @state() private _narrow = false;
  @state() private _drawerOpen = false;
  @state() private _sidebarExpanded = true;
  @state() private _connected = false;

  private _unsubMediaQuery?: () => void;
  private _unsubConnection?: () => void;

  private _applyingUndoRedo = false;
  private _syncingHash = false;

  private _undoRedo = new UndoRedoController<UndoStackItem>({
    currentConfig: () => ({
      viewIndex: this._activeView,
      config: structuredClone(this.lovelace!),
    }),
    apply: (item) => this._applyUndoRedo(item),
    onStackChange: () => this.requestUpdate(),
  });

  private _onKeyDown = (ev: KeyboardEvent): void => {
    if (!this._editMode || this._panel !== "home") return;
    const mod = ev.ctrlKey || ev.metaKey;
    if (!mod) return;
    if (ev.key === "z" && !ev.shiftKey) {
      ev.preventDefault();
      this._undo();
    } else if (ev.key === "y" || (ev.key === "z" && ev.shiftKey)) {
      ev.preventDefault();
      this._redo();
    }
  };

  private _onShowConfirm = (ev: Event): void => {
    this._confirmParams = (ev as CustomEvent<ConfirmDialogParams>).detail;
    this._confirmOpen = true;
  };

  private _onShowDeleteCard = (ev: Event): void => {
    const detail = (ev as CustomEvent<{ cardConfig?: LovelaceCardConfig }>).detail;
    this._deleteCardConfig = detail.cardConfig;
    this._deleteCardOpen = true;
  };

  private _onLayoutEditEnabled = (ev: Event): void => {
    const enabled = Boolean((ev as CustomEvent<{ enabled?: boolean }>).detail?.enabled);
    this._layoutEditEnabled = enabled;
    if (!enabled && this._editMode) {
      this._exitEditMode();
    }
  };

  connectedCallback(): void {
    super.connectedCallback();
    try {
      const stored = localStorage.getItem(SIDEBAR_EXPANDED_KEY);
      if (stored !== null) this._sidebarExpanded = stored === "true";
    } catch {
      // ignore
    }
    this._layoutEditEnabled = isLayoutEditEnabled();
    document.addEventListener(LAYOUT_EDIT_ENABLED_EVENT, this._onLayoutEditEnabled);
    this._unsubMediaQuery = listenMediaQuery(NARROW_QUERY, (matches) => {
      this._narrow = matches;
      if (!matches) this._drawerOpen = false;
    });
    document.addEventListener("flow-show-confirm", this._onShowConfirm);
    document.addEventListener("flow-show-delete-card", this._onShowDeleteCard);
    this.addEventListener("add-section", this._onAddSection as EventListener);
    this.addEventListener("add-card", this._onAddCard as EventListener);
    this.addEventListener("layout-will-change", this._onLayoutWillChange as EventListener);
    this.addEventListener("layout-changed", this._onLayoutChanged as EventListener);
    this.addEventListener("ll-edit-card", this._onEditCard as EventListener);
    this.addEventListener("ll-duplicate-card", this._onDuplicateCard as EventListener);
    this.addEventListener("ll-copy-card", this._onCopyCard as EventListener);
    this.addEventListener("ll-delete-card", this._onDeleteCard);
    this.addEventListener("ll-edit-section", this._onEditSection as EventListener);
    this.addEventListener("ll-duplicate-section", this._onDuplicateSection as EventListener);
    this.addEventListener("ll-delete-section", this._onDeleteSection);
    this.addEventListener("hass-toggle-menu", this._onToggleMenu as EventListener);
    this.addEventListener("hass-dock-sidebar", this._onDockSidebar as EventListener);
    window.addEventListener("keydown", this._onKeyDown);
    window.addEventListener("hashchange", this._onHashChange);
    this._syncViewFromHash();
  }

  disconnectedCallback(): void {
    this._unsubMediaQuery?.();
    this._unsubConnection?.();
    document.removeEventListener("flow-show-confirm", this._onShowConfirm);
    document.removeEventListener("flow-show-delete-card", this._onShowDeleteCard);
    document.removeEventListener(LAYOUT_EDIT_ENABLED_EVENT, this._onLayoutEditEnabled);
    this.removeEventListener("add-section", this._onAddSection as EventListener);
    this.removeEventListener("add-card", this._onAddCard as EventListener);
    this.removeEventListener("layout-will-change", this._onLayoutWillChange as EventListener);
    this.removeEventListener("layout-changed", this._onLayoutChanged as EventListener);
    this.removeEventListener("ll-edit-card", this._onEditCard as EventListener);
    this.removeEventListener("ll-duplicate-card", this._onDuplicateCard as EventListener);
    this.removeEventListener("ll-copy-card", this._onCopyCard as EventListener);
    this.removeEventListener("ll-delete-card", this._onDeleteCard);
    this.removeEventListener("ll-edit-section", this._onEditSection as EventListener);
    this.removeEventListener("ll-duplicate-section", this._onDuplicateSection as EventListener);
    this.removeEventListener("ll-delete-section", this._onDeleteSection);
    this.removeEventListener("hass-toggle-menu", this._onToggleMenu as EventListener);
    this.removeEventListener("hass-dock-sidebar", this._onDockSidebar as EventListener);
    window.removeEventListener("keydown", this._onKeyDown);
    window.removeEventListener("hashchange", this._onHashChange);
    super.disconnectedCallback();
  }

  protected updated(changed: PropertyValues): void {
    this.toggleAttribute("expanded", this._sidebarExpanded && !this._narrow);
    this.toggleAttribute("modal", this._narrow);
    if (changed.has("flow")) {
      this._bindConnection();
    }
    if (changed.has("lovelace") && this.lovelace) {
      this._clampActiveView();
      this._syncViewFromHash();
    }
  }

  private _view() {
    if (!this.lovelace) return undefined;
    return getActiveView(this.lovelace, this._activeView);
  }

  private _clampActiveView(): void {
    if (!this.lovelace?.views.length) {
      this._activeView = 0;
      return;
    }
    if (this._activeView >= this.lovelace.views.length) {
      this._activeView = this.lovelace.views.length - 1;
    }
    if (this._activeView < 0) this._activeView = 0;
  }

  /** Parse `#/path` or `#path` → path segment (HA-style). */
  private _hashViewPath(): string | undefined {
    const raw = location.hash.replace(/^#\/?/, "").trim();
    if (!raw) return undefined;
    const segment = decodeURIComponent(raw.split(/[/?#]/)[0] ?? "");
    if (!segment || segment === "config" || segment === "settings") return undefined;
    return segment;
  }

  private _onHashChange = (): void => {
    if (this._syncingHash) return;
    this._syncViewFromHash();
  };

  private _syncViewFromHash(): void {
    if (!this.lovelace) return;
    const path = this._hashViewPath();
    if (path === undefined) {
      this._writeViewHash(this._activeView, true);
      return;
    }
    const index = resolveViewIndex(this.lovelace, path);
    if (index !== this._activeView) {
      this._activeView = index;
      this._closeEditors();
    }
  }

  private _writeViewHash(index: number, replace = false): void {
    if (!this.lovelace) return;
    const view = this.lovelace.views[index];
    if (!view) return;
    const next = `#/${viewUrlPath(view, index)}`;
    if (location.hash === next) return;
    this._syncingHash = true;
    if (replace) {
      history.replaceState(null, "", `${location.pathname}${location.search}${next}`);
    } else {
      location.hash = next;
    }
    queueMicrotask(() => {
      this._syncingHash = false;
    });
  }

  private _selectView(index: number, options?: { replaceHash?: boolean }): void {
    if (!this.lovelace) return;
    const clamped = resolveViewIndex(this.lovelace, index);
    if (clamped === this._activeView) {
      this._writeViewHash(clamped, options?.replaceHash ?? false);
      return;
    }
    this._activeView = clamped;
    this._closeEditors();
    this._writeViewHash(clamped, options?.replaceHash ?? false);
  }

  private _closeEditors(): void {
    this._pickerOpen = false;
    this._pickerSection = undefined;
    this._cardEditorOpen = false;
    this._cardEditorTarget = undefined;
    this._sectionEditorOpen = false;
    this._sectionEditorTarget = undefined;
    this._viewEditorOpen = false;
    this._viewEditorTarget = undefined;
    this._viewEditorIsNew = false;
  }

  private _reservedViewPaths(excludeIndex?: number): string[] {
    if (!this.lovelace) return [];
    return this.lovelace.views
      .map((view, index) => (index === excludeIndex ? undefined : view.path))
      .filter((path): path is string => Boolean(path));
  }

  private _onViewSelected = (ev: CustomEvent<{ index: number }>): void => {
    this._panel = "home";
    this._selectView(ev.detail.index);
  };

  private _onViewAdd = (): void => {
    if (!this.lovelace || !this._editMode) return;
    this._viewEditorIsNew = true;
    this._viewEditorTarget = undefined;
    this._viewEditorOpen = true;
  };

  private _onViewEdit = (ev: CustomEvent<{ index: number }>): void => {
    if (!this.lovelace || !this._editMode) return;
    this._viewEditorIsNew = false;
    this._viewEditorTarget = ev.detail.index;
    this._viewEditorOpen = true;
  };

  private _onViewDelete = (ev: CustomEvent<{ index: number }>): void => {
    void this._deleteView(ev.detail.index);
  };

  private _saveViewEdit(ev: CustomEvent<{ config: LovelaceViewConfig; isNew: boolean }>): void {
    if (!this.lovelace || !this._editMode) return;
    const draft = ev.detail.config;
    this._commitBeforeChange();

    if (ev.detail.isNew) {
      const view = generateDefaultView({
        title: draft.title,
        path: draft.path,
        icon: draft.icon,
      });
      view.max_columns = draft.max_columns ?? 4;
      if (draft.dense_section_placement) {
        view.dense_section_placement = true;
      }
      const index = addView(this.lovelace, view);
      this._viewEditorOpen = false;
      this._viewEditorIsNew = false;
      this._viewEditorTarget = undefined;
      this._notifyLayoutChanged();
      this._selectView(index);
      return;
    }

    const index = this._viewEditorTarget ?? this._activeView;
    updateView(this.lovelace, index, {
      title: draft.title,
      path: draft.path,
      icon: draft.icon,
      max_columns: draft.max_columns,
      dense_section_placement: draft.dense_section_placement,
    });
    // Clear dense flag when unchecked
    if (!draft.dense_section_placement) {
      const view = this.lovelace.views[index];
      if (view) delete view.dense_section_placement;
    }
    this._viewEditorOpen = false;
    this._viewEditorTarget = undefined;
    this._notifyLayoutChanged();
    this._writeViewHash(index, true);
  }

  private async _deleteView(index: number): Promise<void> {
    if (!this.lovelace || !this._editMode) return;
    if (this.lovelace.views.length <= 1) return;
    const view = this.lovelace.views[index];
    if (!view) return;
    const title = view.title || view.path || `视图 ${index + 1}`;
    const confirmed = await showConfirmationDialog({
      title: "删除视图",
      text: `确定删除「${title}」及其所有分区与卡片？`,
      confirmText: "删除",
      destructive: true,
    });
    if (!confirmed) return;
    this._commitBeforeChange();
    const nextIndex = deleteView(this.lovelace, index);
    this._notifyLayoutChanged();
    this._selectView(nextIndex, { replaceHash: true });
  }

  private _commitBeforeChange(): void {
    if (!this._editMode || !this.lovelace || this._applyingUndoRedo) return;
    this._undoRedo.commit({
      viewIndex: this._activeView,
      config: structuredClone(this.lovelace),
    });
    this.requestUpdate();
  }

  private _notifyLayoutChanged(): void {
    this._layoutRevision += 1;
    void this._persist();
  }

  private _onLayoutWillChange = (): void => {
    this._commitBeforeChange();
  };

  private _onLayoutChanged = (): void => {
    this._notifyLayoutChanged();
  };

  private _onPanelSelected = (ev: CustomEvent<{ panel: FlowPanel }>): void => {
    if (this._editMode) {
      this._editMode = false;
      this._undoRedo.reset();
    }
    this._panel = ev.detail.panel;
    if (this._narrow) this._drawerOpen = false;
    if (this._panel === "home") {
      this._writeViewHash(this._activeView, true);
    }
  };

  private _onToggleMenu = (ev: Event): void => {
    const detail = (ev as CustomEvent<{ open?: boolean }>).detail;
    if (this._narrow) {
      this._drawerOpen = detail?.open ?? !this._drawerOpen;
      return;
    }
    this._sidebarExpanded = detail?.open ?? !this._sidebarExpanded;
    try {
      localStorage.setItem(SIDEBAR_EXPANDED_KEY, String(this._sidebarExpanded));
    } catch {
      // ignore
    }
  };

  private _onDockSidebar = (ev: Event): void => {
    const dock = (ev as CustomEvent<{ dock: "docked" | "auto" }>).detail?.dock;
    this._sidebarExpanded = dock === "docked";
  };

  private _headerTitle(viewTitle: string): string {
    if (this._panel === "config") return "设置";
    return viewTitle;
  }

  private _applyUndoRedo(item: UndoStackItem): void {
    this._applyingUndoRedo = true;
    this.lovelace = structuredClone(item.config);
    this._activeView = item.viewIndex;
    this._clampActiveView();
    this._layoutRevision += 1;
    this._applyingUndoRedo = false;
    this._writeViewHash(this._activeView, true);
    void this._persist();
  }

  private _undo(): void {
    this._undoRedo.undo();
  }

  private _redo(): void {
    this._undoRedo.redo();
  }

  private _onAddSection = (): void => {
    const view = this._view();
    if (!view) return;
    this._commitBeforeChange();
    addSection(view, generateDefaultSection(true));
    this._notifyLayoutChanged();
  };

  private _onAddCard = (ev: CustomEvent<{ section: LovelaceSectionConfig }>): void => {
    this._pickerSection = ev.detail.section;
    this._pickerOpen = true;
  };

  private _onEditCard = (ev: CustomEvent<CardEditTarget>): void => {
    this._cardEditorTarget = ev.detail;
    this._cardEditorOpen = true;
  };

  private _onDuplicateCard = (ev: CustomEvent<CardEditTarget>): void => {
    const view = this._view();
    if (!view) return;
    const section = view.sections[ev.detail.sectionIndex];
    if (!section) return;
    this._commitBeforeChange();
    replaceSection(
      view,
      ev.detail.sectionIndex,
      duplicateCard(section, ev.detail.cardIndex),
    );
    this._notifyLayoutChanged();
  };

  private _onCopyCard = (ev: CustomEvent<CardEditTarget>): void => {
    const view = this._view();
    if (!view) return;
    const section = view.sections[ev.detail.sectionIndex];
    const card = section?.cards[ev.detail.cardIndex];
    if (!card) return;
    setCardClipboard(structuredClone(card));
  };

  private _onDeleteCard = (ev: Event): void => {
    void this._deleteCard(ev as CustomEvent<CardEditTarget & { silent?: boolean }>);
  };

  private async _deleteCard(
    ev: CustomEvent<CardEditTarget & { silent?: boolean }>,
  ): Promise<void> {
    const view = this._view();
    if (!view) return;
    const section = view.sections[ev.detail.sectionIndex];
    if (!section) return;
    const cardConfig = section.cards[ev.detail.cardIndex];
    if (!ev.detail.silent) {
      const confirmed = await showDeleteCardDialog(cardConfig);
      if (!confirmed) return;
    }
    this._commitBeforeChange();
    replaceSection(
      view,
      ev.detail.sectionIndex,
      deleteCard(section, ev.detail.cardIndex),
    );
    this._notifyLayoutChanged();
  }

  private _onConfirmResult(ev: CustomEvent<{ confirmed: boolean }>): void {
    this._confirmOpen = false;
    this._confirmParams = undefined;
    resolveConfirmationDialog(ev.detail.confirmed);
  }

  private _onDeleteCardResult(ev: CustomEvent<{ confirmed: boolean }>): void {
    this._deleteCardOpen = false;
    this._deleteCardConfig = undefined;
    resolveDeleteCardDialog(ev.detail.confirmed);
  }

  private _onEditSection = (ev: CustomEvent<{ sectionIndex: number }>): void => {
    this._sectionEditorTarget = ev.detail.sectionIndex;
    this._sectionEditorOpen = true;
  };

  private _onDuplicateSection = (ev: CustomEvent<{ sectionIndex: number }>): void => {
    const view = this._view();
    if (!view) return;
    this._commitBeforeChange();
    duplicateSection(view, ev.detail.sectionIndex);
    this._notifyLayoutChanged();
  };

  private _onDeleteSection = (ev: Event): void => {
    void this._deleteSection(ev as CustomEvent<{ sectionIndex: number }>);
  };

  private async _deleteSection(
    ev: CustomEvent<{ sectionIndex: number }>,
  ): Promise<void> {
    const view = this._view();
    if (!view) return;
    const section = view.sections[ev.detail.sectionIndex];
    if (!section) return;

    const cardCount = section.cards?.length ?? 0;
    if (cardCount > 0) {
      const confirmed = await showConfirmationDialog({
        title: "删除部件",
        text: "此部件及其所有卡片都将被删除。",
        confirmText: "删除",
        destructive: true,
      });
      if (!confirmed) return;
    }

    this._commitBeforeChange();
    deleteSection(view, ev.detail.sectionIndex);
    this._notifyLayoutChanged();
  }

  private async _pickCard(
    detail: { type: string; config?: LovelaceCardConfig },
  ): Promise<void> {
    const view = this._view();
    if (!this._pickerSection || !view) return;
    const sectionIndex = view.sections.indexOf(this._pickerSection);
    if (sectionIndex < 0) return;

    let stub: LovelaceCardConfig;
    if (detail.config) {
      stub = structuredClone(detail.config);
    } else {
      const ctor = await ensureCardLoaded(detail.type);
      stub = (ctor?.getStubConfig?.() ?? { type: detail.type }) as LovelaceCardConfig;
    }

    pushRecentCardType(stub.type);
    this._commitBeforeChange();
    replaceSection(view, sectionIndex, addCard(this._pickerSection, stub));
    this._pickerOpen = false;
    this._pickerSection = undefined;
    this._notifyLayoutChanged();
  }

  private async _saveCardEdit(ev: CustomEvent<{ config: LovelaceCardConfig }>): Promise<void> {
    const view = this._view();
    const target = this._cardEditorTarget;
    if (!view || !target) return;
    const section = view.sections[target.sectionIndex];
    if (!section) return;
    this._commitBeforeChange();
    replaceSection(
      view,
      target.sectionIndex,
      updateCard(section, target.cardIndex, ev.detail.config),
    );
    this._cardEditorOpen = false;
    this._cardEditorTarget = undefined;
    this._notifyLayoutChanged();
  }

  private _saveSectionEdit(ev: CustomEvent<{ patch: Partial<LovelaceSectionConfig> }>): void {
    const view = this._view();
    const sectionIndex = this._sectionEditorTarget;
    if (!view || sectionIndex === undefined) return;
    this._commitBeforeChange();
    updateSection(view, sectionIndex, ev.detail.patch);
    this._sectionEditorOpen = false;
    this._sectionEditorTarget = undefined;
    this._notifyLayoutChanged();
  }

  private async _persist(): Promise<void> {
    if (!this.lovelace) return;
    await saveLayout(this.lovelace);
    const addresses = collectEntities(this.lovelace);
    this.flow.connection.trackAddresses(addresses);
    this.flow.connection.query(addresses);
  }

  private _enterEditMode(): void {
    if (!this._layoutEditEnabled) return;
    this._editMode = true;
    this._undoRedo.reset();
  }

  private _exitEditMode(): void {
    this._editMode = false;
    this._undoRedo.reset();
  }

  private _bindConnection(): void {
    this._unsubConnection?.();
    this._unsubConnection = undefined;
    if (!this.flow?.connection) {
      this._connected = false;
      return;
    }
    this._connected = this.flow.connected;
    this._unsubConnection = this.flow.connection.subscribeConnection((connected) => {
      this._connected = connected;
    });
  }

  protected render() {
    if (!this.lovelace || !this.flow) return nothing;
    const views = this.lovelace.views;
    const active = views[this._activeView];
    const cardEditorSection =
      this._cardEditorTarget && active
        ? active.sections[this._cardEditorTarget.sectionIndex]
        : undefined;
    const cardEditorConfig =
      cardEditorSection && this._cardEditorTarget
        ? cardEditorSection.cards[this._cardEditorTarget.cardIndex]
        : undefined;
    const sectionEditorConfig =
      this._sectionEditorTarget !== undefined && active
        ? active.sections[this._sectionEditorTarget]
        : undefined;
    const viewTitle = active?.title ?? active?.path ?? this.lovelace.title ?? "Flow";
    const headerTitle = this._headerTitle(viewTitle);
    const showEditActions = this._panel === "home";
    const editMode = showEditActions && this._editMode;
    const hasTabViews = views.length > 1;
    const showViewTabs = showEditActions && (editMode || hasTabViews);

    return html`
      <ha-drawer
        class="shell"
        .type=${this._narrow ? "modal" : ""}
        .open=${this._drawerOpen}
        @hass-drawer-closed=${() => {
          this._drawerOpen = false;
        }}
      >
        <ha-sidebar
          slot="sidebar"
          .activePanel=${this._panel}
          .connected=${this._connected}
          .narrow=${this._narrow}
          @panel-selected=${this._onPanelSelected}
        ></ha-sidebar>

        <div class="main ${this._narrow ? "narrow" : ""} ${editMode ? "edit-mode" : ""}">
          <header class="header">
            <div class="toolbar">
              <ha-menu-button
                .narrow=${this._narrow}
                .hasNotifications=${!this._connected}
              ></ha-menu-button>
              <div class="main-title">
                ${showViewTabs && !editMode
                  ? html`
                      <flow-view-tabs
                        .views=${views}
                        .activeIndex=${this._activeView}
                        @view-selected=${this._onViewSelected}
                      ></flow-view-tabs>
                    `
                  : html`
                      <span>${headerTitle}</span>
                      ${editMode
                        ? html`<ha-icon icon="mdi:pencil" class="edit-pencil"></ha-icon>`
                        : nothing}
                    `}
              </div>
              <div class="action-items">
                <span
                  class="conn-status ${this._connected ? "online" : "offline"}"
                  title=${this._connected ? "已连接到 Flow" : "未连接到 Flow（演示模式）"}
                  role="status"
                  aria-live="polite"
                >
                  <ha-svg-icon
                    .path=${this._connected ? mdiLanConnect : mdiLanDisconnect}
                  ></ha-svg-icon>
                </span>
                ${showEditActions && this._layoutEditEnabled
                  ? editMode
                    ? html`
                        <ha-icon-button
                          .path=${mdiUndo}
                          label="撤销"
                          .disabled=${!this._undoRedo.canUndo}
                          @click=${this._undo}
                        ></ha-icon-button>
                        <ha-icon-button
                          .path=${mdiRedo}
                          label="重做"
                          .disabled=${!this._undoRedo.canRedo}
                          @click=${this._redo}
                        ></ha-icon-button>
                        <ha-icon-button
                          .path=${mdiCheck}
                          label="完成"
                          @click=${this._exitEditMode}
                        ></ha-icon-button>
                      `
                    : html`
                        <ha-icon-button
                          .path=${mdiPencil}
                          label="编辑"
                          @click=${this._enterEditMode}
                        ></ha-icon-button>
                      `
                  : nothing}
              </div>
            </div>
            ${editMode
              ? html`
                  <div class="edit-tabs">
                    <flow-view-tabs
                      .views=${views}
                      .activeIndex=${this._activeView}
                      editMode
                      @view-selected=${this._onViewSelected}
                      @view-add=${this._onViewAdd}
                      @view-edit=${this._onViewEdit}
                      @view-delete=${this._onViewDelete}
                    ></flow-view-tabs>
                  </div>
                `
              : nothing}
          </header>
          <div class="content">
            ${this._panel === "home" && active
              ? html`
                  <flow-view
                    .config=${active}
                    .viewIndex=${this._activeView}
                    .editMode=${this._editMode}
                    .layoutRevision=${this._layoutRevision}
                  ></flow-view>
                `
              : this._panel === "config"
                ? html`<flow-settings-panel .flow=${this.flow}></flow-settings-panel>`
                : nothing}
          </div>
        </div>
      </ha-drawer>

      <flow-card-picker
          .open=${this._pickerOpen}
          .flow=${this.flow}
          @close=${() => {
            this._pickerOpen = false;
            this._pickerSection = undefined;
          }}
          @card-picked=${(ev: CustomEvent<{ type: string; config?: LovelaceCardConfig }>) =>
            void this._pickCard(ev.detail)}
        ></flow-card-picker>

        <flow-card-editor
          .open=${this._cardEditorOpen}
          .config=${cardEditorConfig}
          .sectionConfig=${cardEditorSection}
          @close=${() => {
            this._cardEditorOpen = false;
            this._cardEditorTarget = undefined;
          }}
          @save=${this._saveCardEdit}
        ></flow-card-editor>

        <flow-section-editor
          .open=${this._sectionEditorOpen}
          .config=${sectionEditorConfig}
          .maxColumns=${active?.max_columns ?? 4}
          @close=${() => {
            this._sectionEditorOpen = false;
            this._sectionEditorTarget = undefined;
          }}
          @save=${this._saveSectionEdit}
        ></flow-section-editor>

        <flow-view-editor
          .open=${this._viewEditorOpen}
          .isNew=${this._viewEditorIsNew}
          .config=${this._viewEditorIsNew
            ? generateDefaultView({
                title: "新建视图",
                path: "",
                icon: "mdi:view-dashboard",
              })
            : this.lovelace.views[this._viewEditorTarget ?? this._activeView]}
          .reservedPaths=${this._reservedViewPaths(
            this._viewEditorIsNew ? undefined : (this._viewEditorTarget ?? this._activeView),
          )}
          @close=${() => {
            this._viewEditorOpen = false;
            this._viewEditorTarget = undefined;
            this._viewEditorIsNew = false;
          }}
          @save=${this._saveViewEdit}
        ></flow-view-editor>

        <flow-confirm-dialog
          .open=${this._confirmOpen}
          .title=${this._confirmParams?.title ?? ""}
          .text=${this._confirmParams?.text ?? ""}
          .confirmText=${this._confirmParams?.confirmText ?? "删除"}
          .cancelText=${this._confirmParams?.cancelText ?? "取消"}
          .destructive=${this._confirmParams?.destructive ?? false}
          @dialog-result=${this._onConfirmResult}
        ></flow-confirm-dialog>

        <flow-delete-card-dialog
          .open=${this._deleteCardOpen}
          .cardConfig=${this._deleteCardConfig}
          @dialog-result=${this._onDeleteCardResult}
        ></flow-delete-card-dialog>
    `;
  }

  static styles = css`
    :host {
      display: block;
      height: 100vh;
      min-height: 100vh;
      overflow: hidden;
      background: var(--primary-background-color, #fafafa);
      color: var(--primary-text-color, #1e293b);
      font-family: var(--ha-font-family, "Segoe UI", system-ui, sans-serif);
      --ha-sidebar-width: calc(56px + var(--safe-area-inset-left, 0px));
      --ha-top-app-bar-width: calc(100% - var(--ha-sidebar-width));
    }
    :host([expanded]) {
      --ha-sidebar-width: calc(256px + var(--safe-area-inset-left, 0px));
    }
    :host([modal]) {
      --ha-sidebar-width: 0px;
      --ha-top-app-bar-width: 100%;
    }
    ha-drawer {
      flex: 1;
      min-width: 0;
      min-height: 0;
      width: 100%;
      height: 100%;
    }
    .main {
      flex: 1;
      min-width: 0;
      display: flex;
      flex-direction: column;
      min-height: 0;
      height: 100%;
      overflow: hidden;
    }
    .content {
      flex: 1;
      min-width: 0;
      min-height: 0;
      overflow: auto;
      display: flex;
      flex-direction: column;
    }
    .header {
      background-color: var(--app-header-background-color, var(--primary-background-color));
      color: var(--app-header-text-color, var(--primary-text-color));
      border-bottom: var(--app-header-border-bottom, 1px solid var(--divider-color));
      padding-top: var(--safe-area-inset-top, 0px);
      flex-shrink: 0;
      z-index: 4;
    }
    .edit-mode .header {
      background-color: var(--app-header-edit-background-color, #455a64);
      color: var(--app-header-edit-text-color, #fff);
      border-bottom: none;
    }
    .toolbar {
      height: var(--header-height, 56px);
      display: flex;
      align-items: center;
      font-size: 20px;
      padding: 0 12px;
      font-weight: 400;
      box-sizing: border-box;
    }
    .narrow .toolbar {
      padding: 0 4px;
    }
    .main-title {
      margin-inline-start: 24px;
      line-height: 1.4;
      flex-grow: 1;
      min-width: 0;
      display: flex;
      align-items: center;
      gap: 8px;
      overflow: hidden;
      white-space: nowrap;
      text-overflow: ellipsis;
    }
    .main-title flow-view-tabs {
      flex: 1;
      min-width: 0;
      height: 100%;
      align-self: stretch;
    }
    .edit-tabs {
      display: flex;
      align-items: stretch;
      min-height: 48px;
      padding: 0 8px 0 12px;
      border-top: 1px solid rgba(255, 255, 255, 0.12);
      --app-header-text-color: var(--app-header-edit-text-color, #fff);
      --ha-tab-indicator-color: var(--app-header-edit-text-color, #fff);
    }
    .edit-tabs flow-view-tabs {
      flex: 1;
      min-width: 0;
    }
    .narrow .main-title {
      margin-inline-start: 8px;
    }
    .edit-pencil {
      color: var(--accent-color, #ff9800);
      width: 18px;
      height: 18px;
      flex-shrink: 0;
    }
    .action-items {
      display: flex;
      align-items: center;
      flex-shrink: 0;
      white-space: nowrap;
      gap: 0;
    }
    .conn-status {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 40px;
      height: 40px;
      flex-shrink: 0;
      color: inherit;
    }
    .conn-status ha-svg-icon {
      width: 24px;
      height: 24px;
      flex-shrink: 0;
    }
    .conn-status.online {
      color: #2e7d32;
    }
    .conn-status.offline {
      color: #c62828;
    }
    .edit-mode .conn-status.online {
      color: #a5d6a7;
    }
    .edit-mode .conn-status.offline {
      color: #ef9a9a;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flow-app": FlowApp;
  }
}
