# -*- coding: utf-8 -*-
# sim_serial_test.py —— 数据序列化/反序列化联测（display 侧四条链）
#
# Part A  TimelineDoc JSON 持久化往返（standalone）：
#         POST /api/v1/timelines → GET 回读逐字段比对 → 磁盘 <id>.json
#         → PUT(乐观锁 rev 0→1，旧 rev 409) → 重启 display → 磁盘装载还原
# Part B  .show 工程导入：ShowDocument JSON(snake_case+pb 枚举) →
#         /api/v1/timelines/import → ShowFileIO 反序列化+R1~R8 校验 →
#         sceneToDoc → 注册；坏 JSON → 400 fail-fast
# Part C  .show 导出回环：GET /api/v1/timelines/<id>/export → 解析比对
# Part D  Scene pb 全字段反序列化（sync 模式）：optional opacity/anchor/
#         corner×8/crop×4/rot_x_y/tween v4/TextPayload 全字段/ScreenWarp/
#         音频无条件依赖/auto-size 依赖判定；坏 pb → 场景解析失败 fail-fast
# 退出码 0 = 全部通过。

import socket, struct, sys, os, time, json, threading, zlib, subprocess
import http.server, http.client

BIN     = r"D:\19_NodeEditorCPP\NodeEditorCPP\build\bin"
EXE     = os.path.join(BIN, "media_display.exe")
WORK    = r"D:\19_NodeEditorCPP\NodeEditorCPP\build\test_serial"
LIB     = os.path.join(WORK, "library")
TLDIR   = os.path.join(WORK, "timelines")
CFG_INI = os.path.join(BIN, "config.ini")

HTTP_PORT   = 8995    # standalone 控制面
SYNC_ASSETS = os.path.join(WORK, "sync_assets")
SYNC_HTTP   = 17003   # Part D 素材 HTTP

fails = []
def check(name, ok, detail=""):
    print("  [%s] %s%s" % ("PASS" if ok else "FAIL", name,
                           ("  | " + detail) if detail else ""))
    sys.stdout.flush()
    if not ok:
        fails.append(name)

def http_json(method, path, body=None, timeout=10):
    c = http.client.HTTPConnection("127.0.0.1", HTTP_PORT, timeout=timeout)
    payload = None
    headers = {}
    if body is not None:
        payload = body if isinstance(body, bytes) else json.dumps(
            body, ensure_ascii=False).encode("utf-8")
        headers["Content-Type"] = "application/json"
    c.request(method, path, payload, headers)
    r = c.getresponse()
    raw = r.read()
    c.close()
    try:
        return r.status, json.loads(raw.decode("utf-8"))
    except ValueError:
        return r.status, {"_raw": raw.decode("utf-8", "replace")}

# ---------------------------------------------------------------------------
# protobuf wire helpers（Part D 用，同 sim_editor_v2.py）
# ---------------------------------------------------------------------------
def varint(n):
    out = b""
    n &= (1 << 64) - 1
    while True:
        b7 = n & 0x7F
        n >>= 7
        if n: out += bytes([b7 | 0x80])
        else: return out + bytes([b7])
def tag(f, wt): return varint((f << 3) | wt)
def ld(f, p):   return tag(f, 2) + varint(len(p)) + p
def vint(f, v): return tag(f, 0) + varint(v)
def s(f, t):    return ld(f, t.encode("utf-8"))
def dbl(f, v):  return tag(f, 1) + struct.pack("<d", v)
def frame(p):   return struct.pack("<I", len(p)) + p
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
            raise ValueError("wt %d" % wt)
    return fields
def sub(fields, fn):
    for f, w, v in fields:
        if f == fn and w == 2:
            return parse_fields(v)
    return None

# ---------------------------------------------------------------------------
# 通用：display 拉起/等待/收尾
# ---------------------------------------------------------------------------
procs = []
def launch_display(mode, log_name, extra):
    out_log = open(os.path.join(WORK, log_name + "_out.log"), "wb")
    err_log = open(os.path.join(WORK, log_name + "_err.log"), "wb")
    p = subprocess.Popen(
        [EXE, "--mode=" + mode, "--no-preview", "--quit-after-ms=120000"]
        + extra, stdout=out_log, stderr=err_log, cwd=WORK)
    procs.append(p)
    return p

def read_disp_log(log_name):
    txt = ""
    for half in ("_out.log", "_err.log"):
        p = os.path.join(WORK, log_name + half)
        if os.path.exists(p):
            with open(p, "rb") as f:
                txt += f.read().decode("utf-8", "replace")
    return txt

def kill_all():
    for p in procs:
        if p.poll() is None:
            p.terminate()
    for p in procs:
        try:
            p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            p.kill()
    procs.clear()

os.makedirs(LIB, exist_ok=True)
os.makedirs(TLDIR, exist_ok=True)
os.makedirs(SYNC_ASSETS, exist_ok=True)
cfg_before = None
if os.path.exists(CFG_INI):
    with open(CFG_INI, "rb") as f:
        cfg_before = f.read()

# ===========================================================================
# Part A+B+C —— standalone 模式（文档持久化 / .show 导入 / 导出回环）
# ===========================================================================
print("== Part A/B/C: standalone 文档持久化 + .show 导入导出 ==")
p1 = launch_display("standalone", "sa1",
                    ["--assets-root=" + LIB, "--timelines-dir=" + TLDIR,
                     "--http-port=%d" % HTTP_PORT, "--id=player-1"])

ready = False
for _ in range(60):
    try:
        st, body = http_json("GET", "/api/v1/status", timeout=2)
        ready = (st == 200 and body.get("ok") is True)
        if ready: break
    except OSError:
        pass
    time.sleep(0.3)
check("standalone 控制面 8995 就绪", ready)

# ---- A1: POST 完整 TimelineDoc ----
DOC = {
    "name": "序列化往返A",
    "loop": True,
    "stop_position_us": 8000000,
    "scene": {
        "canvas_w": 1280.0, "canvas_h": 720.0,
        "tracks": [{
            "name": "轨-1",
            "clips": [
                {"text": "序列化ABSeq\n第二行", "start_us": 0,
                 "duration_us": 5000000, "in_us": 0,
                 "x": 10.5, "y": 20.25, "w": 300.0, "h": 100.0,
                 "opacity": 0.75, "z": 5, "rotation": 15.5,
                 "fade_in_us": 250000.0, "fade_out_us": 500000.0,
                 "crop_l": 3.0, "crop_r": 4.0, "crop_t": 5.0, "crop_b": 6.0},
                {"file": "media/a.png", "start_us": 5000000,
                 "duration_us": 3000000, "x": 0.0, "y": 0.0,
                 "w": 640.0, "h": 360.0, "opacity": 0.5, "z": 1},
            ],
        }],
    },
}
st, body = http_json("POST", "/api/v1/timelines", DOC)
tid = (body.get("data") or {}).get("id", "")
check("A1 POST 创建 (rev=0)",
      st == 200 and body.get("ok") is True and bool(tid)
      and (body.get("data") or {}).get("rev") == 0,
      "status=%s id=%r" % (st, tid))

# ---- A2: GET 单条回读逐字段比对 ----
st, body = http_json("GET", "/api/v1/timelines/" + tid)
d = body.get("data") or {}
same = (st == 200 and d.get("name") == DOC["name"]
        and d.get("loop") is True
        and d.get("stop_position_us") == 8000000
        and d.get("rev") == 0)
sc = d.get("scene") or {}
same = same and sc.get("canvas_w") == 1280.0 and sc.get("canvas_h") == 720.0
tr = (sc.get("tracks") or [{}])[0]
same = same and tr.get("name") == "轨-1" and len(tr.get("clips") or []) == 2
if same:
    c0 = tr["clips"][0]
    same = (c0.get("text") == "序列化ABSeq\n第二行"
            and c0.get("start_us") == 0 and c0.get("duration_us") == 5000000
            and c0.get("x") == 10.5 and c0.get("y") == 20.25
            and c0.get("w") == 300.0 and c0.get("h") == 100.0
            and c0.get("opacity") == 0.75 and c0.get("z") == 5
            and c0.get("rotation") == 15.5
            and c0.get("fade_in_us") == 250000.0
            and c0.get("fade_out_us") == 500000.0
            and c0.get("crop_l") == 3.0 and c0.get("crop_r") == 4.0
            and c0.get("crop_t") == 5.0 and c0.get("crop_b") == 6.0)
    c1 = tr["clips"][1]
    same = same and c1.get("file") == "media/a.png" \
        and c1.get("opacity") == 0.5 and c1.get("duration_us") == 3000000
check("A2 GET 回读全字段一致（往返幂等）", same,
      "keys=%s" % sorted(d.keys())[:8])

# ---- A3: 列表 duration（stop_position_us 优先）----
st, body = http_json("GET", "/api/v1/timelines")
lst = body.get("data") or []
ent = next((e for e in lst if e.get("id") == tid), None)
check("A3 列表含条目且 duration_us=stop(8s)", ent is not None
      and ent.get("duration_us") == 8000000,
      "n=%d dur=%s" % (len(lst), ent and ent.get("duration_us")))

# ---- A4: 磁盘文档 ----
disk_path = os.path.join(TLDIR, tid + ".json")
ok_disk = False
if os.path.exists(disk_path):
    with open(disk_path, "r", encoding="utf-8") as f:
        disk = json.load(f)
    ok_disk = disk.get("name") == "序列化往返A" \
        and disk.get("stop_position_us") == 8000000
check("A4 磁盘 <id>.json 落盘且可反解", ok_disk, disk_path)

# ---- A5: PUT 乐观锁 rev 0→1；旧 rev 409 ----
DOC2 = json.loads(json.dumps(DOC))
DOC2["name"] = "序列化往返A-v2"
DOC2["loop"] = False
DOC2["rev"] = 0
st, body = http_json("PUT", "/api/v1/timelines/" + tid, DOC2)
ok_put = st == 200 and (body.get("data") or {}).get("rev") == 1
st2, body2 = http_json("PUT", "/api/v1/timelines/" + tid, DOC2)  # 旧 rev=0
check("A5 PUT rev 乐观锁（0→1 成功，重放 409）",
      ok_put and st2 == 409, "put=%s replay=%s" % (st, st2))

# ---- B: .show 导入 ----
SHOW = {
    "scene": {
        "id": "sc-show-1", "version": "v1",
        "canvas_w": "1920", "canvas_h": "1080",
        "screens": [{"id": "scr-1", "player_id": "player-x",
                     "x": "0", "y": "0", "w": "960", "h": "540"}],
        "timeline": {
            "duration_us": "6000000",
            "tracks": [{"id": "t1", "name": "T", "kind": "ASSET_TEXT",
                        "clips": [{"id": "c1", "asset_id": "tx1",
                                   "start_us": "0", "duration_us": "6000000",
                                   "transform": {"x": "50", "y": "60",
                                                 "w": "400", "h": "80",
                                                 "opacity": 0.9, "z": "2",
                                                 "rotation": "12.5"}}]}],
        },
        "assets": [{"id": "tx1", "kind": "ASSET_TEXT",
                    "text": {"content": "SHOW 导入文本", "font_size": "48",
                             "color": "16711680", "bold": True}}],
    },
    "displays": [{"display_id": "player-1", "host": "127.0.0.1",
                  "port": "17100", "enabled": True}],
}
show_text = json.dumps(SHOW, ensure_ascii=False)
st, body = http_json("POST", "/api/v1/timelines/import", show_text.encode("utf-8"))
sid = (body.get("data") or {}).get("id", "")
check("B1 .show 导入 200（JSON→pb→SceneLoader→doc）",
      st == 200 and body.get("ok") is True and bool(sid),
      "status=%s resp=%s" % (st, json.dumps(body, ensure_ascii=False)[:160]))

st, body = http_json("GET", "/api/v1/timelines/" + sid)
d = body.get("data") or {}
tr = ((d.get("scene") or {}).get("tracks") or [{}])[0]
c0 = (tr.get("clips") or [{}])[0]
ok_show = (d.get("name") == "import" and c0.get("text") == "SHOW 导入文本"
           and c0.get("x") == 50.0 and c0.get("opacity") == 0.9
           and c0.get("duration_us") == 6000000)
st2, body2 = http_json("GET", "/api/v1/timelines")
ent2 = next((e for e in (body2.get("data") or [])
             if e.get("id") == sid), None)
check("B2 导入产物回读（文本/变换/时长映射 sceneToDoc）",
      ok_show and ent2 is not None and ent2.get("duration_us") == 6000000,
      "dur=%s text=%r" % (ent2 and ent2.get("duration_us"),
                          c0.get("text")))

# ---- B3: 坏 .show fail-fast ----
st, body = http_json("POST", "/api/v1/timelines/import",
                     b'{"scene": {"id": "broken", ')
check("B3 截断 .show → 400（反序列化防线）",
      st == 400 and body.get("ok") is False, "status=%s" % st)

# ---- C: .show 导出回环 ----
import urllib.request
req = urllib.request.Request(
    "http://127.0.0.1:%d/api/v1/timelines/%s/export" % (HTTP_PORT, sid))
try:
    with urllib.request.urlopen(req, timeout=10) as r:
        exported = r.read().decode("utf-8")
        disp = r.headers.get("Content-Disposition", "")
    ex = json.loads(exported)
    esc = ex.get("scene") or {}
    etrs = esc.get("timeline", {}).get("tracks", [])
    # 文本载荷在 scene.assets[].text（Asset 的 oneof payload），不在 clip 上
    etexts = [a.get("text", {}).get("content")
              for a in esc.get("assets", []) if a.get("text")]
    try:
        cw = float(esc.get("canvas_w"))
    except (TypeError, ValueError):
        cw = -1.0
    ok_exp = (cw == 1920.0 and "SHOW 导入文本" in etexts
              and "attachment" in disp and sid in disp)
    check("C1 .show 导出回环（doc→Scene→pb JSON）", ok_exp,
          "canvas_w=%r tracks=%d assets=%d texts=%s"
          % (esc.get("canvas_w"), len(etrs), len(esc.get("assets", [])),
             etexts))
except Exception as e:
    check("C1 .show 导出回环（doc→Scene→pb JSON）", False, repr(e))

# ---- A6: 重启 display → 磁盘装载还原 ----
kill_all()
time.sleep(0.8)
p2 = launch_display("standalone", "sa2",
                    ["--assets-root=" + LIB, "--timelines-dir=" + TLDIR,
                     "--http-port=%d" % HTTP_PORT, "--id=player-1"])
ready = False
for _ in range(60):
    try:
        st, body = http_json("GET", "/api/v1/status", timeout=2)
        ready = (st == 200)
        if ready: break
    except OSError:
        pass
    time.sleep(0.3)
check("A6-1 display 重启完成", ready)

st, body = http_json("GET", "/api/v1/timelines/" + tid)
d = body.get("data") or {}
tr = (d.get("scene") or {}).get("tracks") or []
c0 = (tr[0].get("clips") or [{}])[0] if tr else {}
ok_reload = (d.get("name") == "序列化往返A-v2" and d.get("rev") == 1
             and d.get("loop") is False
             and c0.get("text") == "序列化ABSeq\n第二行"
             and c0.get("rotation") == 15.5 and c0.get("crop_l") == 3.0)
st2, body2 = http_json("GET", "/api/v1/timelines")
ids = [e.get("id") for e in (body2.get("data") or [])]
check("A6-2 重启后磁盘装载还原（内容+rev 完整）",
      ok_reload and tid in ids and sid in ids,
      "ids=%s" % ids)
sid_alive = sid in ids

# ===========================================================================
# Part D —— sync 模式 Scene pb 全字段反序列化
# ===========================================================================
print("== Part D: sync 模式 Scene pb 全字段反序列化 ==")

# 素材料 + 内嵌 HTTP
FILE_V = bytes((i * 11 + 7) & 0xFF for i in range(200 * 1024))
FILE_A = bytes((i * 3 + 1) & 0xFF for i in range(64 * 1024))
CRC_V = "%08x" % (zlib.crc32(FILE_V) & 0xFFFFFFFF)
CRC_A = "%08x" % (zlib.crc32(FILE_A) & 0xFFFFFFFF)
http_hits = []

class SyncHandler(http.server.BaseHTTPRequestHandler):
    FILES = {"/asset/vid-1": (FILE_V, CRC_V), "/asset/aud-1": (FILE_A, CRC_A)}
    def _resp(self, with_body):
        http_hits.append((self.command, self.path))
        item = self.FILES.get(self.path)
        if item is None:
            self.send_response(404); self.end_headers(); return
        data, crc = item
        self.send_response(200)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("X-Asset-Size", str(len(data)))
        self.send_header("X-Asset-Crc32", crc)
        self.end_headers()
        if with_body:
            self.wfile.write(data)
    def do_GET(self):  self._resp(True)
    def do_HEAD(self): self._resp(False)
    def log_message(self, *a): pass

srv = http.server.ThreadingHTTPServer(("127.0.0.1", SYNC_HTTP), SyncHandler)
threading.Thread(target=srv.serve_forever, daemon=True).start()

# sync 实例前先杀 standalone 实例（其 8995 控制面会让 sync 实例
# "Address already in use" exit 2——控制面两模式共用端口）
kill_all()
time.sleep(0.5)
p3 = launch_display("sync", "sy1",
                    ["--id=player-1", "--port=17100",
                     "--assets-root=" + SYNC_ASSETS,
                     "--http-port=0"])   # sync 测试不需要控制面
sock = None
for _ in range(60):
    try:
        sock = socket.create_connection(("127.0.0.1", 17100), timeout=2)
        break
    except OSError:
        if p3.poll() is not None: break
        time.sleep(0.3)

if sock is None:
    check("D0 display(sync) 监听 17100", False, "未监听")
else:
    raw = read_frame(sock)
    reg = sub(parse_fields(raw or b""), 1)
    check("D1 Register#1", reg is not None)
    ack = (vint(1, 1) + s(2, "s-1") + s(4, "127.0.0.1")
           + vint(5, 17001) + vint(6, SYNC_HTTP))   # 不指派（id=player-1）
    sock.sendall(frame(ld(2, ack)))

    # ---- 全字段 Scene pb ----
    # TweenChannel：Position 三维 SMOOTH / Size 二维 / Rotation 三维 /
    # Opacity 首尾淡 4 键 / AudioGain HOLD
    # 【铁律】prop 是枚举=varint 字段，直接 vint(1,N)；写成 ld(1,vint(1,N))
    # 会变成 wire type 2 → 解析器按未知字段跳过 → prop=UNKNOWN → R8 拒绝
    def kf(t, v0, v1=0.0, v2=0.0, interp=0):
        return (vint(1, t) + dbl(2, v0) + dbl(3, v1) + dbl(5, v2)
                + vint(4, interp))
    tw_pos = vint(1, 1) + ld(2, kf(0, 100, 200, 0, 2)) \
        + ld(2, kf(5000000, 300, 400, 1, 2))
    tw_size = vint(1, 2) + ld(2, kf(0, 640, 360)) \
        + ld(2, kf(5000000, 800, 450))
    tw_rot = vint(1, 3) + ld(2, kf(0, 5, -5, 10, 1)) \
        + ld(2, kf(5000000, 0, 0, 0, 1))
    tw_op = vint(1, 4) + ld(2, kf(0, 0)) + ld(2, kf(250000, 1)) \
        + ld(2, kf(4750000, 1)) + ld(2, kf(5000000, 0))
    tw_gain = vint(1, 16) + ld(2, kf(0, 1.25, 0, 0, 1))

    tf_full = (dbl(1, 100.0) + dbl(2, 100.0)          # x y（w/h 缺省=auto-size）
               + dbl(5, 10.0)                          # rotation
               + dbl(6, 0.5)                           # optional opacity 显式
               + vint(7, 3) + vint(8, 2)               # z anchor
               + dbl(9, 1.0) + dbl(9, 2.0) + dbl(9, -1.5) + dbl(9, 0.5)
               + dbl(9, 1.0) + dbl(9, 2.0) + dbl(9, -1.5) + dbl(9, 0.5)
                                                       # corner ×8
               + dbl(10, 5.0) + dbl(11, -5.0)          # rot_x rot_y
               + dbl(12, 1.0) + dbl(13, 2.0) + dbl(14, 3.0) + dbl(15, 4.0))
    tf_audio = dbl(1, 0.0) + dbl(2, 0.0)

    asset_v = (s(1, "vid-1") + vint(2, 1) + s(3, "media/auto.mp4")
               + vint(4, len(FILE_V)) + s(5, "crc32:" + CRC_V)
               + vint(6, 5000000) + vint(7, 640) + vint(8, 360)
               + dbl(9, 30.0))
    asset_a = (s(1, "aud-1") + vint(2, 4) + s(3, "media/sfx.wav")
               + vint(4, len(FILE_A)) + s(5, "crc32:" + CRC_A))
    asset_t = (s(1, "tx-1") + vint(2, 3)
               + ld(20, s(1, "全字段文本") + s(2, "Microsoft YaHei")
                    + vint(3, 42) + vint(4, 0xFF00FF) + vint(5, 1)
                    + vint(6, 1) + vint(7, 1) + vint(9, 0x203040)))

    clip1 = (s(1, "c1") + s(2, "vid-1") + vint(3, 0) + vint(4, 5000000)
             + ld(6, tf_full) + ld(7, tw_pos) + ld(7, tw_size)
             + ld(7, tw_rot) + ld(7, tw_op) + ld(7, tw_gain)
             + vint(8, 4) + dbl(9, 1.25))
    clip2 = (s(1, "c2") + s(2, "aud-1") + vint(3, 0) + vint(4, 5000000)
             + ld(6, tf_audio))
    clip3 = (s(1, "c3") + s(2, "tx-1") + vint(3, 0) + vint(4, 5000000)
             + ld(6, dbl(1, 500.0) + dbl(2, 300.0)))
    track1 = (s(1, "t1") + s(2, "V") + vint(3, 1) + ld(4, clip1))
    track2 = (s(1, "t2") + s(2, "A") + vint(3, 4) + ld(4, clip2))
    track3 = (s(1, "t3") + s(2, "T") + vint(3, 3) + ld(4, clip3))
    timeline = vint(1, 5000000) + ld(2, track1) + ld(2, track2) \
        + ld(2, track3)
    warp = vint(1, 1) + vint(2, 2) + vint(3, 2) \
        + dbl(4, 0.0) + dbl(4, 0.0) + dbl(4, 1.0) + dbl(4, 0.0) \
        + dbl(4, 0.0) + dbl(4, 1.0) + dbl(4, 1.0) + dbl(4, 1.0)
    screen1 = (s(1, "scr-1") + s(2, "player-1") + dbl(3, 0.0) + dbl(4, 0.0)
               + dbl(5, 960.0) + dbl(6, 540.0) + ld(9, warp))
    scene_pb = (s(1, "sc-full") + s(2, "v9") + dbl(3, 1920.0) + dbl(4, 1080.0)
                + ld(5, screen1) + ld(6, timeline)
                + ld(7, asset_v) + ld(7, asset_a) + ld(7, asset_t))

    sock.sendall(frame(ld(3, s(1, "sc-full") + s(2, "v9") + ld(3, scene_pb))))

    def wait_hits(paths, timeout=15):
        dl = time.time() + timeout
        while time.time() < dl:
            if all(any(m == "GET" and p == q for m, p in http_hits)
                   for q in paths):
                return True
            time.sleep(0.1)
        return False

    check("D2 全字段场景解析成功+素材拉取（auto-size 相交）",
          wait_hits(["/asset/vid-1"]),
          "hits=%s" % http_hits)
    check("D3 音频素材无条件依赖拉取", wait_hits(["/asset/aud-1"]))
    check("D4 文本素材零下载（随场景分发）",
          not any(p == "/asset/tx-1" for _, p in http_hits))

    # ---- 坏场景 fail-fast + 状态不坏 ----
    sock.sendall(frame(ld(3, s(1, "sc-bad") + s(2, "v0")
                            + ld(3, scene_pb[: len(scene_pb) // 2]))))
    time.sleep(0.6)
    # 再发最小合法场景确认解析器未坏
    mini = (s(1, "sc-mini") + s(2, "v1") + dbl(3, 1920.0) + dbl(4, 1080.0))
    sock.sendall(frame(ld(3, s(1, "sc-mini") + s(2, "v1") + ld(3, mini))))
    time.sleep(0.6)
    logs = read_disp_log("sy1")
    check("D5 坏 pb → 场景解析失败（fail-fast）",
          "场景解析失败" in logs, "")
    check("D6 坏场景后解析器未坏（sc-mini 正常接收）",
          "id=sc-mini" in logs and "解析=" in logs)

    # ---- PLAY/STOP ----
    anchor_us = int(time.time() * 1_000_000) + 300_000
    sock.sendall(frame(ld(4, vint(1, anchor_us) + vint(2, 0)
                            + vint(3, 1) + s(4, "s-1"))))
    def wait_ack(timeout=6):
        dl = time.time() + timeout
        while time.time() < dl:
            try:
                raw = read_frame(sock, timeout=1)
            except OSError:
                raw = None
            if raw is None: continue
            for f, w, v in parse_fields(raw):
                if f == 5 and w == 2:
                    return any(fn == 2 and wt == 0 and val == 1
                               for fn, wt, val in parse_fields(v))
        return False
    ok_play = wait_ack()
    sock.sendall(frame(ld(4, vint(1, int(time.time() * 1_000_000) + 200_000)
                            + vint(2, 0) + vint(3, 4) + s(4, "s-1"))))
    ok_stop = wait_ack()
    check("D7 全字段场景 PLAY/STOP Ack", ok_play and ok_stop)
    try:
        sock.close()
    except OSError:
        pass

srv.shutdown()

# ===========================================================================
# 汇总收尾
# ===========================================================================
kill_all()
if cfg_before is not None:
    with open(CFG_INI, "wb") as f:
        f.write(cfg_before)
elif os.path.exists(CFG_INI):
    os.remove(CFG_INI)

print("-" * 64)
if fails:
    print("[sim] %d 项失败: %s" % (len(fails), "; ".join(fails)))
    sys.exit(1)
print("[sim] 序列化/反序列化全部通过"
      "（文档持久化往返/重启装载/.show 导入导出/Scene pb 全字段）")
sys.exit(0)
