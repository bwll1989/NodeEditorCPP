# sim_editor.py —— 模拟 Editor 与 media_display（sync 模式）全链路握手
#
# 帧格式：4 字节大端长度前缀 + sync.proto Envelope（手写 protobuf wire
# format，零依赖）。验证路径：
#   1) TCP 连 17100 → 收 display 的 MsgRegister（身份先行）
#   2) 回 MsgRegisterAck（accepted + session_id）
#   3) 发 MsgScene（最小合法 Scene pb：id/version/canvas，空屏空时间轴）
#   4) 发 MsgReAnchor(CMD_PLAY，锚点 = now + 300ms)
#   5) 收 MsgAck（display 应用锚点后回执）
#   6) 发 MsgReAnchor(CMD_STOP) → 收 Ack → 断开
# 每步打 [PASS]/[FAIL]，退出码 0=全过。

import socket, struct, sys, time

HOST, PORT = "127.0.0.1", 17100
fails = []

def varint(n):
    out = b""
    n &= (1 << 64) - 1          # 负数按二补码 64 位编码
    while True:
        b7 = n & 0x7F
        n >>= 7
        if n:
            out += bytes([b7 | 0x80])
        else:
            return out + bytes([b7])

def tag(field, wt): return varint((field << 3) | wt)
def ld(field, payload): return tag(field, 2) + varint(len(payload)) + payload
def vint(field, value): return tag(field, 0) + varint(value)
def s(field, text): return ld(field, text.encode("utf-8"))
def dbl(field, value): return tag(field, 1) + struct.pack("<d", value)
def frame(payload):
    # FrameCodec::encode = putU32Le + payload —— 长度前缀【小端】
    return struct.pack("<I", len(payload)) + payload

def read_frame(sock, timeout=5):
    sock.settimeout(timeout)
    hdr = b""
    while len(hdr) < 4:
        chunk = sock.recv(4 - len(hdr))
        if not chunk:
            return None
        hdr += chunk
    (n,) = struct.unpack("<I", hdr)
    data = b""
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            return None
        data += chunk
    return data

def parse_fields(data):
    """极简 protobuf 解码：返回 [(field_number, wire_type, value)]"""
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
            raise ValueError(f"wire type {wt}")
    return fields

def sub(fields, fn):
    """取嵌套消息字段（length-delimited）并解码"""
    for f, w, v in fields:
        if f == fn and w == 2:
            return parse_fields(v)
    return None

def check(name, ok, detail=""):
    print(f"  [{'PASS' if ok else 'FAIL'}] {name}" + (f" — {detail}" if detail else ""))
    if not ok:
        fails.append(name)

# ---------------------------------------------------------------------------
sock = socket.create_connection((HOST, PORT), timeout=5)
print("[sim] TCP 已连接 17100")

# 1) 收 MsgRegister
raw = read_frame(sock)
if raw is None:
    print("[sim] 致命：连接后未收到任何帧（display 未发 Register）")
    sys.exit(1)
env = parse_fields(raw)
reg = sub(env, 1)
check("display 先行上报 MsgRegister", reg is not None)
pid = ver = ""
udp_port = 0
for f, w, v in (reg or []):
    if f == 1: pid = v.decode()
    if f == 2: ver = v.decode()
    if f == 3: udp_port = v
check("MsgRegister.player_id 可解析", bool(pid), f"id={pid!r} version={ver!r} udp_report_port={udp_port}")

# 2) 回 MsgRegisterAck{accepted=1, session_id="s-1"}（udp/http 端口 0=未启用）
ack_body = vint(1, 1) + s(2, "s-1")
sock.sendall(frame(ld(2, ack_body)))
print("[sim] 已回 MsgRegisterAck(accepted=true, session='s-1')")

# 3) 发 MsgScene：最小合法 Scene{id,version,canvas_w,canvas_h}
scene_pb = (s(1, "sc-sim-1") + s(2, "v1")
            + dbl(3, 1920.0) + dbl(4, 1080.0))
sock.sendall(frame(ld(3, s(1, "sc-sim-1") + s(2, "v1") + ld(3, scene_pb))))
print("[sim] 已发 MsgScene（空场景 sc-sim-1/v1 1920x1080）")

# 4) 发 MsgReAnchor CMD_PLAY（锚点 = now + 300ms，位置 0）
anchor_us = int(time.time() * 1_000_000) + 300_000
ra_body = (vint(1, anchor_us) + vint(2, 0) + vint(3, 1) + s(4, "s-1"))
sock.sendall(frame(ld(4, ra_body)))
print(f"[sim] 已发 MsgReAnchor CMD_PLAY anchor=+300ms")

# 5) 收 MsgAck（Envelope.field5）
got_ack = None
deadline = time.time() + 5
while time.time() < deadline:
    raw = read_frame(sock, timeout=2)
    if raw is None:
        continue
    for f, w, v in parse_fields(raw):
        if f == 5 and w == 2:
            got_ack = parse_fields(v)
    if got_ack:
        break
ok_detail = False
if got_ack:
    ok_flag = any(f == 2 and w == 0 and v == 1 for f, w, v in got_ack)
    detail = next((v.decode() for f, w, v in got_ack if f == 3 and w == 2), "")
    ok_detail = ok_flag
    check("收到 MsgAck(ok=true)", ok_detail, detail)
else:
    check("收到 MsgAck(ok=true)", False, "5s 内无回执")
    fails.pop()  # 已记录，补一条统一信息
    check("锚点命令链路（Register→Ack→Scene→ReAnchor→Ack）", False)
    sock.close()
    sys.exit(1)
check("锚点命令链路（Register→Ack→Scene→ReAnchor→Ack）", True)

# 6) CMD_STOP → 收 Ack
anchor_us2 = int(time.time() * 1_000_000) + 200_000
sock.sendall(frame(ld(4, vint(1, anchor_us2) + vint(2, 0) + vint(3, 4) + s(4, "s-1"))))
got_stop = False
deadline = time.time() + 5
while time.time() < deadline:
    raw = read_frame(sock, timeout=2)
    if raw is None:
        continue
    for f, w, v in parse_fields(raw):
        if f == 5 and w == 2:
            got_stop = True
    if got_stop:
        break
check("CMD_STOP 回执", got_stop)

sock.close()
print(f"[sim] 完成：{'全部通过' if not fails else f'{len(fails)} 项失败'}")
sys.exit(1 if fails else 0)
