import type { FlowConnection, FlowEntityState } from "../types";
import { FlowStore } from "./flow-store";

export class FlowWebSocketConnection implements FlowConnection {
  connected = false;
  private ws: WebSocket | null = null;
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private heartbeatTimer: ReturnType<typeof setInterval> | null = null;
  private readonly store: FlowStore;
  private connectionListeners = new Set<(connected: boolean) => void>();
  private stateListeners = new Set<(states: Record<string, FlowEntityState>) => void>();
  private subscribedAddresses = new Set<string>();

  constructor(store: FlowStore, private readonly url: string) {
    this.store = store;
    this.store.subscribeAll((states) => {
      this.stateListeners.forEach((fn) => fn(states));
    });
  }

  connect(): void {
    if (this.ws) return;
    this.ws = new WebSocket(this.url);

    this.ws.onopen = () => {
      this.connected = true;
      this.notifyConnection();
      this.startHeartbeat();
      if (this.subscribedAddresses.size > 0) {
        this.query([...this.subscribedAddresses]);
      }
    };

    this.ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(String(event.data)) as {
          address?: string;
          addr?: string;
          value?: unknown;
          heartbeat?: boolean;
          query?: unknown;
          event?: string;
          seq?: number;
          timestamp?: string;
          level?: string;
          message?: string;
        };
        if (msg.heartbeat) return;
        if (msg.event === "actions_changed") {
          document.dispatchEvent(new CustomEvent("flow-actions-changed", { detail: msg }));
          return;
        }
        if (msg.event === "log") {
          document.dispatchEvent(
            new CustomEvent("flow-log-entry", {
              detail: {
                seq: Number(msg.seq ?? 0),
                timestamp: String(msg.timestamp ?? ""),
                level: String(msg.level ?? "Info"),
                message: String(msg.message ?? ""),
              },
            }),
          );
          return;
        }
        const address = msg.address || msg.addr;
        if (!address) return;
        this.store.applyUpdate(address, msg.value as never);
        document.dispatchEvent(
          new CustomEvent("flow-ws-message", {
            detail: { address, value: msg.value, raw: msg },
          }),
        );
      } catch {
        // ignore malformed payloads
      }
    };

    this.ws.onclose = () => {
      this.connected = false;
      this.ws = null;
      this.stopHeartbeat();
      this.notifyConnection();
      this.scheduleReconnect();
    };

    this.ws.onerror = () => {
      this.connected = false;
      this.notifyConnection();
    };
  }

  subscribeStates(callback: (states: Record<string, FlowEntityState>) => void): () => void {
    this.stateListeners.add(callback);
    callback(this.store.getStatesRecord());
    return () => this.stateListeners.delete(callback);
  }

  subscribeConnection(callback: (connected: boolean) => void): () => void {
    this.connectionListeners.add(callback);
    callback(this.connected);
    return () => this.connectionListeners.delete(callback);
  }

  trackAddresses(addresses: string[]): void {
    const next = addresses.filter(Boolean);
    next.forEach((addr) => this.subscribedAddresses.add(addr));
    if (this.connected && next.length > 0) {
      this.query(next);
    }
  }

  sendCommand(address: string, value: unknown): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) return;
    this.ws.send(JSON.stringify({ address, value }));
  }

  query(addresses: string[]): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN || addresses.length === 0) return;
    const CHUNK = 128;
    for (let i = 0; i < addresses.length; i += CHUNK) {
      this.ws.send(JSON.stringify({ query: addresses.slice(i, i + CHUNK) }));
    }
  }

  private notifyConnection(): void {
    this.connectionListeners.forEach((fn) => fn(this.connected));
  }

  private scheduleReconnect(): void {
    if (this.reconnectTimer) return;
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.connect();
    }, 3000);
  }

  private startHeartbeat(): void {
    this.stopHeartbeat();
    this.heartbeatTimer = setInterval(() => {
      if (!this.ws || this.ws.readyState !== WebSocket.OPEN) return;
      this.ws.send(JSON.stringify({ heartbeat: true, t: Date.now() }));
    }, 25000);
  }

  private stopHeartbeat(): void {
    if (this.heartbeatTimer) {
      clearInterval(this.heartbeatTimer);
      this.heartbeatTimer = null;
    }
  }
}
