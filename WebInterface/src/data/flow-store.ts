import type { FlowEntityState, FlowValue } from "../types";

type StateListener = (state: FlowEntityState) => void;

export class FlowStore {
  private states = new Map<string, FlowEntityState>();
  private globalListeners = new Set<(states: Record<string, FlowEntityState>) => void>();
  private addressListeners = new Map<string, Set<StateListener>>();

  getStatesRecord(): Record<string, FlowEntityState> {
    const out: Record<string, FlowEntityState> = {};
    this.states.forEach((state, address) => {
      out[address] = state;
    });
    return out;
  }

  getState(address: string): FlowEntityState | undefined {
    return this.states.get(address);
  }

  applyUpdate(address: string, value: FlowValue, attributes: Record<string, unknown> = {}): void {
    const prev = this.states.get(address);
    const state: FlowEntityState = {
      address,
      state: value,
      attributes: { ...prev?.attributes, ...attributes },
      last_changed: Date.now(),
    };
    this.states.set(address, state);
    this.addressListeners.get(address)?.forEach((fn) => fn(state));
    this.notifyGlobal();
  }

  subscribe(address: string, listener: StateListener): () => void {
    if (!this.addressListeners.has(address)) {
      this.addressListeners.set(address, new Set());
    }
    this.addressListeners.get(address)!.add(listener);
    const current = this.states.get(address);
    if (current) listener(current);
    return () => this.addressListeners.get(address)?.delete(listener);
  }

  subscribeAll(listener: (states: Record<string, FlowEntityState>) => void): () => void {
    this.globalListeners.add(listener);
    listener(this.getStatesRecord());
    return () => this.globalListeners.delete(listener);
  }

  private notifyGlobal(): void {
    const snapshot = this.getStatesRecord();
    this.globalListeners.forEach((fn) => fn(snapshot));
  }
}
