const STACK_LIMIT = 75;

export interface UndoRedoControllerOptions<T> {
  stackLimit?: number;
  currentConfig: () => T;
  apply: (config: T) => void;
  onStackChange?: () => void;
}

export class UndoRedoController<T> {
  private _undoStack: T[] = [];

  private _redoStack: T[] = [];

  private readonly _stackLimit: number;

  private readonly _currentConfig: () => T;

  private readonly _apply: (config: T) => void;

  private readonly _onStackChange?: () => void;

  constructor(options: UndoRedoControllerOptions<T>) {
    this._stackLimit = options.stackLimit ?? STACK_LIMIT;
    this._currentConfig = options.currentConfig;
    this._apply = options.apply;
    this._onStackChange = options.onStackChange;
  }

  get canUndo(): boolean {
    return this._undoStack.length > 0;
  }

  get canRedo(): boolean {
    return this._redoStack.length > 0;
  }

  commit(snapshot: T): void {
    if (this._undoStack.length >= this._stackLimit) {
      this._undoStack.shift();
    }
    this._undoStack.push(snapshot);
    this._redoStack = [];
    this._onStackChange?.();
  }

  undo(): void {
    if (this._undoStack.length === 0) return;
    this._redoStack.push(this._currentConfig());
    const snapshot = this._undoStack.pop()!;
    this._apply(snapshot);
    this._onStackChange?.();
  }

  redo(): void {
    if (this._redoStack.length === 0) return;
    this._undoStack.push(this._currentConfig());
    const snapshot = this._redoStack.pop()!;
    this._apply(snapshot);
    this._onStackChange?.();
  }

  reset(): void {
    this._undoStack = [];
    this._redoStack = [];
    this._onStackChange?.();
  }
}
