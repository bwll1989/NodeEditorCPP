# 粘贴到 Dify「代码执行」节点 — 仅标准库，无需 paho-mqtt
# 环境变量：MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS, MQTT_CONTROL_TOPIC

import json
import os
import socket
import struct
import time

MQTT_HOST = os.getenv("MQTT_HOST", "")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_USER = os.getenv("MQTT_USER", "")
MQTT_PASS = os.getenv("MQTT_PASS", "")
CONTROL_TOPIC = os.getenv("MQTT_CONTROL_TOPIC", "flow/control")

COMMANDS = {
    "start_background_music": {"address": "/dataflow/Dataflow/0/play", "value": 1},
    "stop_background_music": {"address": "/dataflow/Dataflow/0/stop", "value": 1},
    "set_background_music_volume": {
        "address": "/dataflow/Dataflow/0/volume",
        "arg": "arg_number",
        "clamp": (-40, 10),
    },
    "set_background_music_loop": {
        "address": "/dataflow/Dataflow/0/loop",
        "arg": "arg_bool",
    },
    "set_light_Brightness": {
        "address": "/dataflow/Dataflow/6/channels0",
        "arg": "arg_number",
        "scale": 2.55,
        "clamp": (0, 255),
    },
}


def _encode_remaining_length(n: int) -> bytes:
    out = bytearray()
    while True:
        digit = n % 128
        n //= 128
        if n > 0:
            digit |= 0x80
        out.append(digit)
        if n == 0:
            break
    return bytes(out)


def _mqtt_string(s: str) -> bytes:
    b = s.encode("utf-8")
    return struct.pack("!H", len(b)) + b


def _mqtt_publish(topic: str, payload: str, host: str, port: int, user: str, password: str, timeout: float = 10.0) -> None:
    topic_b = topic.encode("utf-8")
    payload_b = payload.encode("utf-8")

    # PUBLISH QoS0: type=3, flags=0
    var_header = struct.pack("!H", len(topic_b)) + topic_b
    remaining = len(var_header) + len(payload_b)
    fixed = bytes([0x30]) + _encode_remaining_length(remaining)
    publish_pkt = fixed + var_header + payload_b

    client_id = f"dify_{int(time.time())}"
    proto_name = b"MQTT"
    proto_level = 4  # 3.1.1
    connect_flags = 0x02  # clean session
    if user:
        connect_flags |= 0x80
    if password:
        connect_flags |= 0x40

    variable = (
        _mqtt_string(proto_name.decode())
        + bytes([proto_level, connect_flags])
        + struct.pack("!H", 60)
        + _mqtt_string(client_id)
    )
    if user:
        variable += _mqtt_string(user)
    if password:
        variable += _mqtt_string(password)

    connect_pkt = bytes([0x10]) + _encode_remaining_length(len(variable)) + variable

    sock = socket.create_connection((host, port), timeout=timeout)
    try:
        sock.sendall(connect_pkt)
        # CONNACK: 2 bytes fixed + 2 bytes variable minimum
        sock.settimeout(timeout)
        header = sock.recv(1)
        if not header:
            raise OSError("broker 无 CONNACK")
        rem = 0
        mult = 1
        while True:
            b = sock.recv(1)
            if not b:
                raise OSError("CONNACK 不完整")
            rem += (b[0] & 0x7F) * mult
            mult *= 128
            if (b[0] & 0x80) == 0:
                break
        body = sock.recv(rem) if rem else b""
        if len(body) < 2 or body[1] != 0:
            raise OSError(f"MQTT 连接被拒绝, CONNACK={body.hex()}")

        sock.sendall(publish_pkt)
        sock.sendall(bytes([0xE0, 0x00]))  # DISCONNECT
    finally:
        sock.close()


def main(command: str, arg_number=None, arg_bool=None) -> dict:
    if not MQTT_HOST:
        return {"result": "请配置环境变量 MQTT_HOST"}

    cmd = (command or "").strip()
    spec = COMMANDS.get(cmd)
    if not spec:
        return {"result": f"未知命令: {cmd}"}

    address = spec["address"]
    if "arg" not in spec:
        value = spec["value"]
    elif spec["arg"] == "arg_number":
        if arg_number is None:
            return {"result": f"命令 {cmd} 需要 arg_number"}
        value = float(arg_number)
        if "scale" in spec:
            value = value * spec["scale"]
        if "clamp" in spec:
            lo, hi = spec["clamp"]
            value = max(lo, min(hi, value))
        if cmd == "set_light_Brightness":
            value = int(round(value))
    else:
        if arg_bool is None:
            return {"result": f"命令 {cmd} 需要 arg_bool"}
        value = bool(arg_bool)

    body = json.dumps({"address": address, "value": value}, ensure_ascii=False)
    try:
        _mqtt_publish(CONTROL_TOPIC, body, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS)
        return {"result": f"OK: {body} -> {CONTROL_TOPIC}"}
    except Exception as e:
        return {"result": f"MQTT 失败: {e}"}
