import { css, html, LitElement, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { styleMap } from "lit/directives/style-map.js";
import type { Flow, LovelaceCard, LovelaceCardConfig, LovelaceGridOptions } from "../../../types";
import { computeTileColor } from "../../../common/entity/tile-color";
import { tileCardHostStyle, tileCardStyle } from "../../../styles/tile-card-style";
import "../../../components/ha-tile-container";
import "../../../components/ha-tile-icon";
import "../../../components/ha-tile-info";
import "../../../components/ha-card";

const TARGET_SAMPLE_RATE = 48000;
/** ~20ms @ 48k — one Timeline frame; avoids flooding WS with ~128-sample quanta. */
const SEND_SAMPLES = 960;
/** Drop PCM when WS send buffer exceeds this (bytes) to shed latency on mobile Wi‑Fi. */
const MAX_WS_BUFFERED = 96 * 1024;

const PCM_WORKLET_CODE = `
class PcmCaptureProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this._acc = new Float32Array(2048);
    this._fill = 0;
    this._target = ${SEND_SAMPLES};
    this.port.onmessage = (ev) => {
      const msg = ev.data;
      if (!msg || msg.type !== "flush") return;
      if (this._fill > 0) {
        const out = this._acc.slice(0, this._fill);
        this.port.postMessage(out, [out.buffer]);
        this._fill = 0;
      }
      this.port.postMessage({ type: "flushed" });
    };
  }
  process(inputs) {
    const input = inputs[0];
    if (!input || !input.length) return true;
    const ch0 = input[0];
    if (!ch0 || !ch0.length) return true;
    const n = ch0.length;
    const stereo = input.length > 1 && input[1] && input[1].length === n;
    const ch1 = stereo ? input[1] : null;
    let offset = 0;
    while (offset < n) {
      const space = this._target - this._fill;
      const take = Math.min(space, n - offset);
      for (let i = 0; i < take; i++) {
        const idx = offset + i;
        this._acc[this._fill + i] = stereo
          ? 0.5 * (ch0[idx] + ch1[idx])
          : ch0[idx];
      }
      this._fill += take;
      offset += take;
      if (this._fill >= this._target) {
        const out = this._acc.slice(0, this._target);
        this.port.postMessage(out, [out.buffer]);
        this._fill = 0;
      }
    }
    return true;
  }
}
registerProcessor("pcm-capture-processor", PcmCaptureProcessor);
`;

function resampleTo48k(input: Float32Array, fromRate: number): Float32Array {
  if (!fromRate || Math.abs(fromRate - TARGET_SAMPLE_RATE) < 1) {
    return input;
  }
  const ratio = fromRate / TARGET_SAMPLE_RATE;
  const outLen = Math.max(1, Math.floor(input.length / ratio));
  const out = new Float32Array(outLen);
  for (let i = 0; i < outLen; i++) {
    const src = i * ratio;
    const i0 = Math.floor(src);
    const i1 = Math.min(i0 + 1, input.length - 1);
    const t = src - i0;
    out[i] = input[i0] * (1 - t) + input[i1] * t;
  }
  return out;
}

function resolvePort(value: unknown): number | undefined {
  if (typeof value === "number" && Number.isFinite(value) && value > 0) {
    return Math.round(value);
  }
  if (typeof value === "string" && value.trim()) {
    const n = Number(value);
    if (Number.isFinite(n) && n > 0) return Math.round(n);
  }
  return undefined;
}

type GetUserMediaFn = (constraints: MediaStreamConstraints) => Promise<MediaStream>;

function resolveGetUserMedia(): { getUserMedia?: GetUserMediaFn; reason?: string } {
  const nav = navigator as Navigator & {
    webkitGetUserMedia?: (
      c: MediaStreamConstraints,
      ok: (s: MediaStream) => void,
      err: (e: Error) => void,
    ) => void;
    mozGetUserMedia?: (
      c: MediaStreamConstraints,
      ok: (s: MediaStream) => void,
      err: (e: Error) => void,
    ) => void;
    getUserMedia?: (
      c: MediaStreamConstraints,
      ok: (s: MediaStream) => void,
      err: (e: Error) => void,
    ) => void;
  };

  if (typeof window !== "undefined" && window.isSecureContext === false) {
    return {
      reason: "麦克风需 https 或 localhost（http://局域网IP 不可用）",
    };
  }

  if (nav.mediaDevices) {
    return { getUserMedia: (c) => nav.mediaDevices!.getUserMedia(c) };
  }

  const legacy = nav.webkitGetUserMedia || nav.mozGetUserMedia || nav.getUserMedia;
  if (legacy) {
    return {
      getUserMedia: (constraints) =>
        new Promise<MediaStream>((resolve, reject) => {
          legacy.call(nav, constraints, resolve, reject);
        }),
    };
  }

  return { reason: "浏览器未提供麦克风接口" };
}

@customElement("hui-mic-card")
export class HuiMicCard extends LitElement implements LovelaceCard {
  public static getStubConfig(): LovelaceCardConfig {
    return {
      type: "mic",
      entity: "/demo/webmic/port",
      name: "对讲",
      icon: "mdi:microphone",
    };
  }

  @property({ attribute: false }) public flow?: Flow;

  @state() private _config?: LovelaceCardConfig;

  /** True only after mic + WebSocket are ready — user can speak. */
  @state() private _ready = false;

  /** Press held, still opening mic / WS. */
  @state() private _connecting = false;

  @state() private _error = "";

  private _pointerId?: number;

  /** Finger/mouse is currently held down (PTT intent). */
  private _pressActive = false;

  /** Bumps on each press/release so late async openStream can be ignored. */
  private _pressToken = 0;

  private _ws: WebSocket | null = null;

  private _mediaStream: MediaStream | null = null;

  private _audioCtx: AudioContext | null = null;

  private _workletNode: AudioWorkletNode | null = null;

  private _scriptNode: ScriptProcessorNode | null = null;

  private _sourceNode: MediaStreamAudioSourceNode | null = null;

  private _muteNode: GainNode | null = null;

  private _workletUrl: string | null = null;

  /** Accumulator for resampled PCM before WS send (handles non-48k and ScriptProcessor). */
  private _sendAcc = new Float32Array(SEND_SAMPLES * 2);

  private _sendFill = 0;

  /** Release in progress: still accept PCM until the tail is sent. */
  private _draining = false;

  private _flushDone: (() => void) | null = null;

  private _stopChain: Promise<void> | null = null;

  private _onWindowPointerUp = (ev: PointerEvent) => {
    void this._endPress(ev.pointerId);
  };


  setConfig(config: LovelaceCardConfig): void {
    this._config = config;
    this.requestUpdate();
  }

  getGridOptions(): LovelaceGridOptions {
    const vertical = Boolean(this._config?.vertical);
    const rows = vertical ? 2 : 1;
    return {
      columns: 6,
      rows,
      min_columns: vertical ? 3 : 6,
      min_rows: rows,
    };
  }

  connectedCallback(): void {
    super.connectedCallback();
    const check = resolveGetUserMedia();
    if (check.reason && !this._error) {
      this._error = check.reason;
    }
  }

  disconnectedCallback(): void {
    this._unbindWindowRelease();
    void this._stopTalking();
    super.disconnectedCallback();
  }

  private _bindWindowRelease(): void {
    window.addEventListener("pointerup", this._onWindowPointerUp, true);
    window.addEventListener("pointercancel", this._onWindowPointerUp, true);
  }

  private _unbindWindowRelease(): void {
    window.removeEventListener("pointerup", this._onWindowPointerUp, true);
    window.removeEventListener("pointercancel", this._onWindowPointerUp, true);
  }

  private _portNumber(): number | undefined {
    const entity = this._config?.entity;
    if (entity && this.flow) {
      const fromState = resolvePort(this.flow.states[entity]?.state);
      if (fromState !== undefined) return fromState;
    }
    return resolvePort(this._config?.port);
  }

  private _wsHost(): string {
    const configured = this._config?.host;
    if (typeof configured === "string" && configured.trim()) {
      return configured.trim();
    }
    return location.hostname || "127.0.0.1";
  }

  private async _startTalking(ev: PointerEvent): Promise<void> {
    // Only primary button / touch; ignore hover/right-click.
    if (ev.pointerType === "mouse" && ev.button !== 0) return;
    ev.preventDefault();
    ev.stopPropagation();
    if (this._pressActive) return;

    const mic = resolveGetUserMedia();
    if (!mic.getUserMedia) {
      this._error = mic.reason ?? "无法访问麦克风";
      return;
    }

    const port = this._portNumber();
    if (!port) {
      this._error = "未绑定有效端口";
      return;
    }

    this._pressActive = true;
    this._pointerId = ev.pointerId;
    const token = ++this._pressToken;
    this._error = "";
    this._ready = false;
    this._connecting = true;
    this._bindWindowRelease();

    try {
      (ev.currentTarget as HTMLElement).setPointerCapture(ev.pointerId);
    } catch {
      // ignore
    }

    try {
      await this._openStream(port, mic.getUserMedia, token);
      // Released while awaiting getUserMedia / WS — tear down immediately.
      if (!this._pressActive || token !== this._pressToken) {
        await this._stopTalking();
        return;
      }
      // Connected: now show "can speak" color.
      this._connecting = false;
      this._ready = true;
    } catch (err) {
      const cancelled = err instanceof Error && err.message === "cancelled";
      if (!cancelled && token === this._pressToken) {
        this._error = err instanceof Error ? err.message : "无法开始对讲";
      }
      await this._stopTalking();
    }
  }

  private async _endPress(pointerId?: number): Promise<void> {
    if (!this._pressActive) return;
    if (pointerId !== undefined && this._pointerId !== undefined && pointerId !== this._pointerId) {
      return;
    }
    this._draining = true;
    this._pressActive = false;
    this._pressToken += 1;
    this._unbindWindowRelease();
    await this._stopTalking();
  }

  private async _onPointerUp(ev: PointerEvent): Promise<void> {
    ev.preventDefault();
    ev.stopPropagation();
    await this._endPress(ev.pointerId);
  }

  private _onContextMenu(ev: Event): void {
    ev.preventDefault();
  }

  private _handlePcm(pcm: Float32Array, inputRate: number): void {
    if ((!this._pressActive && !this._draining) || !this._ws || this._ws.readyState !== WebSocket.OPEN) {
      return;
    }

    // While draining the release tail, do not drop — that audio still has to go out.
    if (!this._draining && this._ws.bufferedAmount > MAX_WS_BUFFERED) {
      this._sendFill = 0;
      return;
    }

    const resampled = resampleTo48k(pcm, inputRate);
    if (!resampled.length) return;

    let offset = 0;
    while (offset < resampled.length) {
      const space = SEND_SAMPLES - this._sendFill;
      const take = Math.min(space, resampled.length - offset);
      if (this._sendFill + take > this._sendAcc.length) {
        const grown = new Float32Array(Math.max(this._sendAcc.length * 2, this._sendFill + take));
        grown.set(this._sendAcc.subarray(0, this._sendFill));
        this._sendAcc = grown;
      }
      this._sendAcc.set(resampled.subarray(offset, offset + take), this._sendFill);
      this._sendFill += take;
      offset += take;

      if (this._sendFill >= SEND_SAMPLES) {
        if (!this._draining && this._ws.bufferedAmount > MAX_WS_BUFFERED) {
          this._sendFill = 0;
          return;
        }
        const chunk = this._sendAcc.slice(0, SEND_SAMPLES);
        this._ws.send(chunk.buffer);
        this._sendFill = 0;
      }
    }
  }


  private async _openStream(
    port: number,
    getUserMedia: GetUserMediaFn,
    token: number,
  ): Promise<void> {
    const stream = await getUserMedia({
      audio: {
        channelCount: 1,
        echoCancellation: true,
        noiseSuppression: true,
        autoGainControl: true,
      },
      video: false,
    });
    if (!this._pressActive || token !== this._pressToken) {
      stream.getTracks().forEach((t) => t.stop());
      throw new Error("cancelled");
    }
    this._mediaStream = stream;
    this._sendFill = 0;

    const AudioCtx =
      window.AudioContext ||
      (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
    const ctx = new AudioCtx({ sampleRate: TARGET_SAMPLE_RATE });
    this._audioCtx = ctx;
    if (ctx.state === "suspended") {
      await ctx.resume();
    }
    if (!this._pressActive || token !== this._pressToken) {
      throw new Error("cancelled");
    }

    const source = ctx.createMediaStreamSource(stream);
    this._sourceNode = source;
    const mute = ctx.createGain();
    mute.gain.value = 0;
    this._muteNode = mute;

    const ws = new WebSocket(`ws://${this._wsHost()}:${port}`);
    ws.binaryType = "arraybuffer";
    this._ws = ws;

    await new Promise<void>((resolve, reject) => {
      const onOpen = () => {
        cleanup();
        resolve();
      };
      const onError = () => {
        cleanup();
        reject(new Error(`无法连接 ws://${this._wsHost()}:${port}`));
      };
      const cleanup = () => {
        ws.removeEventListener("open", onOpen);
        ws.removeEventListener("error", onError);
      };
      ws.addEventListener("open", onOpen);
      ws.addEventListener("error", onError);
    });

    if (!this._pressActive || token !== this._pressToken) {
      throw new Error("cancelled");
    }

    const inputRate = ctx.sampleRate;
    let captureReady = false;

    if (ctx.audioWorklet) {
      try {
        this._workletUrl = URL.createObjectURL(
          new Blob([PCM_WORKLET_CODE], { type: "application/javascript" }),
        );
        await ctx.audioWorklet.addModule(this._workletUrl);
        if (!this._pressActive || token !== this._pressToken) {
          throw new Error("cancelled");
        }
        const worklet = new AudioWorkletNode(ctx, "pcm-capture-processor");
        this._workletNode = worklet;
        worklet.port.onmessage = (event: MessageEvent) => {
          const data = event.data as Float32Array | { type?: string };
          if (data && typeof data === "object" && "type" in data && data.type === "flushed") {
            this._flushDone?.();
            this._flushDone = null;
            return;
          }
          this._handlePcm(data as Float32Array, inputRate);
        };
        source.connect(worklet);
        worklet.connect(mute);
        mute.connect(ctx.destination);
        captureReady = true;
      } catch (err) {
        if (err instanceof Error && err.message === "cancelled") throw err;
        // Fall through to ScriptProcessor.
      }
    }

    if (!captureReady) {
      if (!this._pressActive || token !== this._pressToken) {
        throw new Error("cancelled");
      }
      const bufferSize = 2048;
      const script = ctx.createScriptProcessor(bufferSize, 1, 1);
      this._scriptNode = script;
      script.onaudioprocess = (ev) => {
        const input = ev.inputBuffer.getChannelData(0);
        this._handlePcm(input, inputRate);
      };
      source.connect(script);
      script.connect(mute);
      mute.connect(ctx.destination);
    }
  }

  private _sendRemainder(): void {
    if (!this._ws || this._ws.readyState !== WebSocket.OPEN || this._sendFill <= 0) return;
    const chunk = this._sendAcc.slice(0, this._sendFill);
    this._sendFill = 0;
    this._ws.send(chunk.buffer);
  }

  /** Push the worklet tail, then the main-thread remainder, then wait until the socket queue is empty. */
  private async _flushOutgoing(): Promise<void> {
    const worklet = this._workletNode;
    if (worklet) {
      await new Promise<void>((resolve) => {
        const timer = window.setTimeout(() => {
          this._flushDone = null;
          resolve();
        }, 80);
        this._flushDone = () => {
          window.clearTimeout(timer);
          resolve();
        };
        try {
          worklet.port.postMessage({ type: "flush" });
        } catch {
          window.clearTimeout(timer);
          this._flushDone = null;
          resolve();
        }
      });
    }

    if (this._scriptNode) {
      // Let the last ScriptProcessor callback land before we send the tail.
      await new Promise<void>((resolve) => window.setTimeout(resolve, 60));
    }

    this._sendRemainder();
  }

  private async _waitSocketDrain(): Promise<void> {
    const ws = this._ws;
    if (!ws || ws.readyState !== WebSocket.OPEN) return;
    const start = performance.now();
    await new Promise<void>((resolve) => {
      const tick = () => {
        if (
          !this._ws ||
          this._ws.readyState !== WebSocket.OPEN ||
          this._ws.bufferedAmount === 0 ||
          performance.now() - start > 400
        ) {
          resolve();
          return;
        }
        window.setTimeout(tick, 15);
      };
      tick();
    });
  }

  private _stopTalking(): Promise<void> {
    if (this._stopChain) return this._stopChain;
    this._stopChain = this._finishStop().finally(() => {
      this._stopChain = null;
    });
    return this._stopChain;
  }

  private async _finishStop(): Promise<void> {
    this._pointerId = undefined;
    this._ready = false;
    this._connecting = false;

    if (this._draining || this._ws?.readyState === WebSocket.OPEN) {
      this._draining = true;
      await this._flushOutgoing();
    }
    this._draining = false;
    this._sendFill = 0;

    if (this._workletNode) {
      try {
        this._workletNode.port.onmessage = null;
        this._workletNode.disconnect();
      } catch {
        // ignore
      }
      this._workletNode = null;
    }

    if (this._scriptNode) {
      try {
        this._scriptNode.onaudioprocess = null;
        this._scriptNode.disconnect();
      } catch {
        // ignore
      }
      this._scriptNode = null;
    }

    if (this._sourceNode) {
      try {
        this._sourceNode.disconnect();
      } catch {
        // ignore
      }
      this._sourceNode = null;
    }

    if (this._muteNode) {
      try {
        this._muteNode.disconnect();
      } catch {
        // ignore
      }
      this._muteNode = null;
    }

    if (this._audioCtx) {
      try {
        await this._audioCtx.close();
      } catch {
        // ignore
      }
      this._audioCtx = null;
    }

    if (this._workletUrl) {
      URL.revokeObjectURL(this._workletUrl);
      this._workletUrl = null;
    }

    if (this._mediaStream) {
      this._mediaStream.getTracks().forEach((t) => t.stop());
      this._mediaStream = null;
    }

    await this._waitSocketDrain();

    if (this._ws) {
      try {
        this._ws.close();
      } catch {
        // ignore
      }
      this._ws = null;
    }
  }

  protected render() {
    if (!this._config) return nothing;

    const entity = this._config.entity;
    const state = entity && this.flow ? this.flow.states[entity] : undefined;
    const port = this._portNumber();
    const name = String(this._config.name ?? state?.attributes?.friendly_name ?? "对讲");
    const icon = String(this._config.icon ?? "mdi:microphone");
    const vertical = Boolean(this._config.vertical);
    const ready = this._ready;
    const connecting = this._connecting;
    const hasError = Boolean(this._error && !ready && !connecting);
    const micOk = Boolean(resolveGetUserMedia().getUserMedia);
    const secondary = this._error
      ? this._error
      : ready
        ? "可以说话 · 松开结束"
        : connecting
          ? "连接中，请稍候…"
          : !micOk
            ? "麦克风不可用"
            : port
              ? `按住说话 · :${port}`
              : "请绑定 Web Mic /port";

    const colorStyle = styleMap({
      "--tile-color": computeTileColor(ready, this._config.color),
    });

    return html`
      <ha-card
        class=${classMap({
          active: ready,
          ready,
          connecting,
          error: hasError,
        })}
        style=${colorStyle}
        tabindex="-1"
        @pointerdown=${this._startTalking}
        @pointerup=${this._onPointerUp}
        @pointercancel=${this._onPointerUp}
        @lostpointercapture=${this._onPointerUp}
        @contextmenu=${this._onContextMenu}
      >
        <ha-tile-container ?vertical=${vertical} ?fixed-info-height=${vertical}>
          <ha-tile-icon slot="icon" .icon=${icon} .active=${ready} interactive></ha-tile-icon>
          <ha-tile-info slot="info" .primary=${name} .secondary=${secondary}></ha-tile-info>
        </ha-tile-container>
      </ha-card>
    `;
  }

  static styles = [
    tileCardStyle,
    tileCardHostStyle,
    css`
      :host {
        display: block;
        height: 100%;
        min-height: 0;
        touch-action: none;
        user-select: none;
        -webkit-user-select: none;
        -webkit-touch-callout: none;
      }
      ha-card {
        height: 100%;
        min-height: 0;
        cursor: pointer;
        overflow: hidden;
        -webkit-tap-highlight-color: transparent;
        transition:
          box-shadow 160ms ease,
          border-color 160ms ease,
          opacity 120ms ease;
      }
      /* Connecting: pressed but not live yet — muted, no "ready" color. */
      ha-card.connecting {
        opacity: 0.85;
        box-shadow:
          var(--ha-card-box-shadow),
          0 0 0 2px color-mix(in srgb, var(--secondary-text-color) 35%, transparent);
      }
      /* Ready: WS connected — clear go-ahead to speak. */
      ha-card.ready {
        --tile-color: var(--primary-color);
        opacity: 1;
        box-shadow:
          var(--ha-card-box-shadow),
          0 0 0 2px color-mix(in srgb, var(--tile-color, var(--primary-color)) 60%, transparent),
          0 0 14px color-mix(in srgb, var(--tile-color, var(--primary-color)) 25%, transparent);
      }
      ha-card.error {
        border-color: var(--error-color, #f44336);
      }
    `,
  ];
}

declare global {
  interface HTMLElementTagNameMap {
    "hui-mic-card": HuiMicCard;
  }
}
