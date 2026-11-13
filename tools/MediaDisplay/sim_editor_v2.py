# -*- coding: utf-8 -*-
# sim_editor_v2.py —— 素材文件下发 + 指派模型（"客户端添加"）联测
#
# 在 sim_editor.py（握手/场景/重锚链路）基础上扩展：
#   A) 指派模型：Ack#1 携带 assigned_player_id="player-9"（≠ 自报
#      --id=player-1）→ display 采用指派 id 并重新注册（Register#2
#      的 player_id 应为 player-9）。
#   B) 文件下发：内嵌 HTTP 服务（127.0.0.1:17002）供 GET /asset/media-1；
#      MsgScene 携带 screen(player-9) + media-1 相交 clip + media-2
#      不相交 clip → 依赖过滤只拉 media-1；MsgAssetSync 重推清单 →
#      就绪表命中幂等跳过；MsgAssetProgress 回报 finished。
#   C) 落盘校验：display_assets/test.png 字节/CRC32 与源一致；
#      .asset_state.tsv 写入就绪记录；stderr 出现"素材不在本机依赖内"。
#   D) 指派后重锚链路照旧可用（PLAY/STOP Ack）。
#
# 帧格式：4 字节【小端】长度前缀 + sync.proto Envelope（手写 pb wire）。
# 用法：python sim_editor_v2.py   （先启动 media_display 由本脚本拉起）
# 退出码 0 = 全部通过。

import socket, struct, sys, os, time, threading, zlib, subprocess
import http.server

BIN     = r"D:\19_NodeEditorCPP\NodeEditorCPP\build\bin"
EXE     = os.path.join(BIN, "media_display.exe")
WORK    = r"D:\19_NodeEditorCPP\NodeEditorCPP\build\test_asset_sync"
ASSETS  = os.path.join(WORK, "display_assets")
CFG_INI = os.path.join(BIN, "config.ini")

HTTP_PORT = 17002
UDP_PORT  = 17001   # 仅在 Ack 里宣告（display 会往这发心跳，无人收无害）
ASSIGNED  = "player-9"

fails = []
def check(name, ok, detail=""):
    print("  [%s] %s%s" % ("PASS" if ok else "FAIL", name,
                           ("  | " + detail) if detail else ""))
    sys.stdout.flush()
    if not ok:
        fails.append(name)

# ---------------------------------------------------------------------------
# protobuf wire helpers（同 sim_editor.py）
# ---------------------------------------------------------------------------
def varint(n):
    out = b""
    n &= (1 << 64) - 1
    while True:
        b7 = n & 0x7F
        n >>= 7
        if n:
            out += bytes([b7 | 0x80])
        else:
            return out + bytes([b7])

def tag(f, wt): return varint((f << 3) | wt)
def ld(f, p):   return tag(f, 2) + varint(len(p)) + p
def vint(f, v): return tag(f, 0) + varint(v)
def s(f, t):    return ld(f, t.encode("utf-8"))
def dbl(f, v):  return tag(f, 1) + struct.pack("<d", v)
def frame(p):   return struct.pack("<I", len(p)) + p   # 小端长度前缀

def read_frame(sock, timeout=5):
    sock.settimeout(timeout)
    hdr = b""
    while len(hdr) < 4:
        c = sock.recv(4 - len(hdr))
        if not c: return None
        hdr += c
    (n,) = struct.unpack("<I", hdr)
    data = b""
    while len(data) < n:
        c = sock.recv(n - len(data))
        if not c: return None
        data += c
    return data

def parse_fields(data):
    fields, i = [], 0
    while i < len(data):
        key, shift = 0, 0
        while True:
            b = data[i]; i += 1
            key |= (b & 0x7F) << shift; shift += 7
            if not (b & 0x80): break
        fn, wt = key >> 3, key & 7
        if wt == 0:
            v, shift = 0, 0
            while True:
                b = data[i]; i += 1
                v |= (b & 0x7F) << shift; shift += 7
                if not (b & 0x80): break
            fields.append((fn, wt, v))
        elif wt == 2:
            ln, shift = 0, 0
            while True:
                b = data[i]; i += 1
                ln |= (b & 0x7F) << shift; shift += 7
                if not (b & 0x80): break
            fields.append((fn, wt, data[i:i + ln])); i += ln
        elif wt == 1:
            fields.append((fn, wt, data[i:i + 8])); i += 8
        elif wt == 5:
            fields.append((fn, wt, data[i:i + 4])); i += 4
        else:
            raise ValueError("wire type %d" % wt)
    return fields

def sub(fields, fn):
    for f, w, v in fields:
        if f == fn and w == 2:
            return parse_fields(v)
    return None

# ---------------------------------------------------------------------------
# 0) 测试料：300KB 确定性字节 + CRC32
# ---------------------------------------------------------------------------
os.makedirs(ASSETS, exist_ok=True)
FILE_BYTES = bytes((i * 7 + 13) & 0xFF for i in range(300 * 1024))
FILE_CRC   = "%08x" % (zlib.crc32(FILE_BYTES) & 0xFFFFFFFF)

# ---------------------------------------------------------------------------
# 1) 内嵌素材 HTTP 服务（模拟 Editor 端 AssetHttpServer：/asset/<id>）
# ---------------------------------------------------------------------------
http_hits = []   # (method, path, range)

class AssetHandler(http.server.BaseHTTPRequestHandler):
    def _respond(self, with_body):
        rng = self.headers.get("Range")
        http_hits.append((self.command, self.path, rng or ""))
        if self.path != "/asset/media-1":
            self.send_response(404)
            self.end_headers()
            return
        start = 0
        partial = False
        if rng and rng.startswith("bytes="):
            try:
                start = int(rng.split("=")[1].split("-")[0])
                partial = start > 0
            except ValueError:
                start = 0
        if start >= len(FILE_BYTES):
            self.send_response(416)
            self.send_header("Content-Range", "bytes */%d" % len(FILE_BYTES))
            self.end_headers()
            return
        chunk = FILE_BYTES[start:]
        if partial:
            self.send_response(206)
            self.send_header("Content-Range",
                             "bytes %d-%d/%d" % (start, len(FILE_BYTES) - 1,
                                                 len(FILE_BYTES)))
        else:
            self.send_response(200)
        self.send_header("Content-Length", str(len(chunk)))
        self.send_header("X-Asset-Size", str(len(FILE_BYTES)))
        self.send_header("X-Asset-Crc32", FILE_CRC)
        self.end_headers()
        if with_body:
            self.wfile.write(chunk)
    def do_GET(self):  self._respond(True)
    def do_HEAD(self): self._respond(False)
    def log_message(self, *a): pass

srv = http.server.ThreadingHTTPServer(("127.0.0.1", HTTP_PORT), AssetHandler)
threading.Thread(target=srv.serve_forever, daemon=True).start()
print("[sim] 素材 HTTP 服务就绪 127.0.0.1:%d  size=%d crc32=%s"
      % (HTTP_PORT, len(FILE_BYTES), FILE_CRC))

# ---------------------------------------------------------------------------
# 2) 快照 config.ini（指派 id 会写回 [display].player_id，收尾还原）
# ---------------------------------------------------------------------------
cfg_before = None
if os.path.exists(CFG_INI):
    with open(CFG_INI, "rb") as f:
        cfg_before = f.read()

# ---------------------------------------------------------------------------
# 3) 拉起 display（sync 受控模式；日志落文件供断言）
# ---------------------------------------------------------------------------
out_log = open(os.path.join(WORK, "display_out.log"), "wb")
err_log = open(os.path.join(WORK, "display_err.log"), "wb")
proc = subprocess.Popen(
    [EXE, "--mode=sync", "--id=player-1", "--port=17100",
     "--assets-root=" + ASSETS, "--no-preview", "--quit-after-ms=60000"],
    stdout=out_log, stderr=err_log, cwd=WORK)
print("[sim] media_display 已拉起 pid=%d" % proc.pid)

sock = None
deadline = time.time() + 20
while time.time() < deadline:
    try:
        sock = socket.create_connection(("127.0.0.1", 17100), timeout=3)
        break
    except OSError:
        if proc.poll() is not None:
            break
        time.sleep(0.3)
if sock is None:
    print("[sim] 致命：20s 内未能连上 17100（display 未监听或已退出）")
    sys.exit(1)
print("[sim] TCP 已连接 17100")

# ---------------------------------------------------------------------------
# 4) Register#1 → Ack#1（指派 player-9 + http 端口）
# ---------------------------------------------------------------------------
raw = read_frame(sock)
if raw is None:
    print("[sim] 致命：未收到 Register#1")
    proc.terminate(); sys.exit(1)
reg1 = sub(parse_fields(raw), 1)
pid1 = next((v.decode() for f, w, v in (reg1 or []) if f == 1 and w == 2), "")
check("Register#1 自报 id=player-1", pid1 == "player-1", "id=%r" % pid1)

def ack_envelope(session):
    body = (vint(1, 1) + s(2, session) + s(4, "127.0.0.1")
            + vint(5, UDP_PORT) + vint(6, HTTP_PORT) + s(7, ASSIGNED))
    return frame(ld(2, body))

sock.sendall(ack_envelope("s-1"))
print("[sim] Ack#1 已发（accepted + http=%d + 指派 %s）" % (HTTP_PORT, ASSIGNED))

# Register#2：指派采纳后重注册换键
raw2 = read_frame(sock, timeout=8)
pid2 = ""
if raw2 is not None:
    reg2 = sub(parse_fields(raw2), 1)
    pid2 = next((v.decode() for f, w, v in (reg2 or []) if f == 1 and w == 2), "")
check("指派模型：display 采用 player-9 并重发 Register", pid2 == ASSIGNED,
      "Register#2.id=%r" % pid2)
sock.sendall(ack_envelope("s-1"))   # Ack#2（指派 id 已生效，不再触发重注册）

# ---------------------------------------------------------------------------
# 5) MsgScene：screen(player-9) + media-1 相交 clip + media-2 界外 clip
# ---------------------------------------------------------------------------
tf1 = dbl(1, 100.0) + dbl(2, 100.0) + dbl(3, 800.0) + dbl(4, 400.0)    # ∩ screen
tf2 = dbl(1, 1500.0) + dbl(2, 800.0) + dbl(3, 300.0) + dbl(4, 200.0)   # 界外
asset1 = (s(1, "media-1") + vint(2, 2) + s(3, "test.png")
          + vint(4, len(FILE_BYTES)) + s(5, "crc32:" + FILE_CRC)
          + vint(7, 800) + vint(8, 400))
asset2 = (s(1, "media-2") + vint(2, 2) + s(3, "skip.png")
          + vint(4, 1000) + s(5, "crc32:00000000")
          + vint(7, 300) + vint(8, 200))
clip1 = s(1, "c1") + s(2, "media-1") + vint(3, 0) + vint(4, 10_000_000) + ld(6, tf1)
clip2 = s(1, "c2") + s(2, "media-2") + vint(3, 0) + vint(4, 10_000_000) + ld(6, tf2)
track1 = s(1, "t1") + s(2, "V1") + vint(3, 2) + ld(4, clip1) + ld(4, clip2)
timeline = vint(1, 10_000_000) + ld(2, track1)
screen1 = (s(1, "scr-1") + s(2, ASSIGNED) + dbl(3, 0.0) + dbl(4, 0.0)
           + dbl(5, 960.0) + dbl(6, 540.0))
scene_pb = (s(1, "sc-asset-1") + s(2, "v1") + dbl(3, 1920.0) + dbl(4, 1080.0)
            + ld(5, screen1) + ld(6, timeline) + ld(7, asset1) + ld(7, asset2))
sock.sendall(frame(ld(3, s(1, "sc-asset-1") + s(2, "v1") + ld(3, scene_pb))))
print("[sim] MsgScene 已发（sc-asset-1/v1，2 素材 2 片段 1 屏）")

# 等 HTTP GET /asset/media-1 到达
def wait_http_hit(path, timeout=15):
    dl = time.time() + timeout
    while time.time() < dl:
        if any(m == "GET" and p == path for m, p, _ in http_hits):
            return True
        time.sleep(0.1)
    return False

got_get = wait_http_hit("/asset/media-1")
check("display 发起 GET /asset/media-1", got_get,
      "hits=%s" % (http_hits[-4:] if http_hits else "无"))

# ---------------------------------------------------------------------------
# 6) 收 MsgAssetProgress 至 finished
# ---------------------------------------------------------------------------
progress_frames = []
def wait_progress_finished(timeout=20):
    dl = time.time() + timeout
    while time.time() < dl:
        try:
            raw = read_frame(sock, timeout=1)
        except OSError:
            raw = None
        if raw is None:
            if time.time() >= dl:
                break
            continue
        for f, w, v in parse_fields(raw):
            if f == 8 and w == 2:
                pf = parse_fields(v)
                d = {fn: val for fn, wt, val in pf}
                progress_frames.append({
                    "done":    d.get(1, 0),
                    "total":   d.get(2, 0),
                    "path":    (d.get(3, b"") or b"").decode("utf-8", "replace"),
                    "bytes":   d.get(4, 0),
                    "ctotal":  d.get(5, 0),
                    "finished": bool(d.get(6, 0)),
                })
                if progress_frames[-1]["finished"]:
                    return progress_frames[-1]
    return None

fin = wait_progress_finished()
check("MsgAssetProgress.finished 上报", fin is not None,
      "frames=%s" % (progress_frames[-3:] if progress_frames else "无"))
check("进度口径 done/total=1/1（依赖过滤后）",
      fin is not None and fin["done"] == 1 and fin["total"] == 1,
      "final=%s" % fin)

# ---------------------------------------------------------------------------
# 7) MsgAssetSync 清单重推 → 就绪表幂等（零下载二轮 finished）
# ---------------------------------------------------------------------------
def entry(aid, path, size, crc):
    return s(1, aid) + s(2, path) + vint(3, size) + s(4, crc)
sync_body = (ld(1, entry("media-1", "test.png", len(FILE_BYTES),
                         "crc32:" + FILE_CRC))
             + ld(1, entry("media-2", "skip.png", 1000, "crc32:00000000"))
             + s(2, "v1"))
n_hits_before = len(http_hits)
sock.sendall(frame(ld(7, sync_body)))
print("[sim] MsgAssetSync 已发（2 项清单）")
fin2 = wait_progress_finished()
check("清单重推 → 幂等 finished（无新增 HTTP 拉取）",
      fin2 is not None and len(http_hits) == n_hits_before,
      "hits_before=%d hits_now=%d finished=%s"
      % (n_hits_before, len(http_hits), fin2 is not None))

# ---------------------------------------------------------------------------
# 8) 落盘 + 就绪表 + 依赖过滤断言
# ---------------------------------------------------------------------------
saved = os.path.join(ASSETS, "test.png")
ok_file = False
if os.path.exists(saved):
    with open(saved, "rb") as f:
        got = f.read()
    ok_file = (len(got) == len(FILE_BYTES)
               and zlib.crc32(got) & 0xFFFFFFFF == int(FILE_CRC, 16))
check("素材落盘且字节/CRC32 一致", ok_file,
      "path=%s size=%s" % (saved,
                           os.path.getsize(saved) if os.path.exists(saved) else "缺"))

tsv = os.path.join(ASSETS, ".asset_state.tsv")
ok_tsv = os.path.exists(tsv) and "media-1" in open(tsv, "r",
                                                   encoding="utf-8",
                                                   errors="replace").read()
check(".asset_state.tsv 就绪表已写入（跨会话幂等）", ok_tsv)

time.sleep(0.3)
def read_log(name):
    with open(os.path.join(WORK, name), "rb") as f:
        return f.read().decode("utf-8", "replace")   # display 输出为 UTF-8
disp_out = read_log("display_out.log")   # std::printf 诊断在 stdout
disp_err = read_log("display_err.log")   # std::fprintf(stderr) 在 stderr
disp_all = disp_out + disp_err
check("依赖过滤：media-2 被跳过（诊断日志）",
      "素材不在本机依赖内" in disp_all and "media-2" in disp_all,
      "out=%d B err=%d B" % (len(disp_out), len(disp_err)))
check("HTTP 服务未收到 skip.png 请求",
      not any("skip" in p for _, p, _ in http_hits))

# ---------------------------------------------------------------------------
# 9) 指派 id 生效后重锚链路（PLAY → Ack → STOP → Ack）
# ---------------------------------------------------------------------------
anchor_us = int(time.time() * 1_000_000) + 300_000
sock.sendall(frame(ld(4, vint(1, anchor_us) + vint(2, 0) + vint(3, 1)
                        + s(4, "s-1"))))
def wait_ack(timeout=6):
    dl = time.time() + timeout
    while time.time() < dl:
        try:
            raw = read_frame(sock, timeout=1)
        except OSError:
            raw = None
        if raw is None:
            continue
        for f, w, v in parse_fields(raw):
            if f == 5 and w == 2:
                af = parse_fields(v)
                return any(fn == 2 and wt == 0 and val == 1
                           for fn, wt, val in af)
        if time.time() >= dl:
            break
    return False
ok_play = wait_ack()
check("CMD_PLAY 回执 ok=true（指派后链路不受影响）", ok_play)
sock.sendall(frame(ld(4, vint(1, int(time.time() * 1_000_000) + 200_000)
                        + vint(2, 0) + vint(3, 4) + s(4, "s-1"))))
ok_stop = wait_ack()
check("CMD_STOP 回执", ok_stop)

# ---------------------------------------------------------------------------
# 10) 收尾：杀 display、还原 config.ini、汇总结论
# ---------------------------------------------------------------------------
try:
    sock.close()
except OSError:
    pass
proc.terminate()
try:
    proc.wait(timeout=5)
except subprocess.TimeoutExpired:
    proc.kill()
srv.shutdown()

if cfg_before is not None:
    with open(CFG_INI, "wb") as f:
        f.write(cfg_before)
elif os.path.exists(CFG_INI):
    os.remove(CFG_INI)

print("-" * 64)
if fails:
    print("[sim] %d 项失败: %s" % (len(fails), "; ".join(fails)))
    sys.exit(1)
print("[sim] 全部通过（指派模型 + 文件下发 + 依赖过滤 + 幂等 + 重锚）")
sys.exit(0)
