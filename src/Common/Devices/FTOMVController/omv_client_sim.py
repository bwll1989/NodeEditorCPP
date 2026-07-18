#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OMV 安卓播放器客户端模拟器（以 tset.lua 定义为准）

tset.lua 判定规则（对整段收包做 hex 后，下标从 1 起）:
  - 握手:   hex[53:56] == "6E61" ("na")
            设备 ID: hex[63:64] 起，若 hex[65:66]=="22" 则一位，否则两位
  - 播放中: hex[23:24] == "32" 且 hex[67:68] == "30"
  - 已停止: hex[23:24] == "32" 且 hex[67:68] == "32"

注意: tset.lua 内置常量帧的 Length 比实际 Payload 少 1（少计结尾 '}'）。
本程序收包兼容该偏差；发包默认同样采用 Length=实际-1，以便与 Lua 侧一致。

用法:
  python omv_client_sim.py
  python omv_client_sim.py -H 127.0.0.1 -p 9090 -i 12
  python omv_client_sim.py --correct-len   # Length 写真实长度（给严格按长度解帧的对端）

交互: play | stop | hs | status | quit | 十六进制帧
"""

from __future__ import annotations

import argparse
import json
import socket
import struct
import sys
import threading
import time
from typing import Optional


FRAME_HEADER = 0xA0


def to_hex_status(data: bytes) -> str:
    """等价于 tset.lua 的 hex2str：每个字节 → 两位大写十六进制。"""
    return "".join(f"{b:02X}" for b in data)


def hex_bytes(data: bytes) -> str:
    return " ".join(f"{b:02X}" for b in data)


def encode_frame(payload: bytes, *, lua_length: bool = True) -> bytes:
    """
    组包: 0xA0 + be16(Length) + Payload
    lua_length=True 时 Length = len(payload)-1，与 tset.lua 常量一致。
    """
    if not payload:
        raise ValueError("empty payload")
    length = len(payload) - 1 if lua_length else len(payload)
    if length < 0:
        raise ValueError("payload too short")
    return bytes([FRAME_HEADER]) + struct.pack(">H", length) + payload


def try_decode(buffer: bytearray) -> list[tuple[bytes, str, dict]]:
    """
    解帧。兼容 tset.lua Length 少 1 的情况：
    若按 Length 截取后 JSON 不完整，且后面恰好还有字节能补全，则多吃这些字节。
    """
    frames: list[tuple[bytes, str, dict]] = []
    while True:
        header = buffer.find(bytes([FRAME_HEADER]))
        if header < 0:
            buffer.clear()
            break
        if header > 0:
            del buffer[:header]
        if len(buffer) < 3:
            break

        claimed = struct.unpack(">H", buffer[1:3])[0]
        if claimed > 64 * 1024:
            del buffer[0]
            continue

        # 先按声明长度取；不够则等待
        need = 3 + claimed
        if len(buffer) < need:
            break

        payload = bytes(buffer[3:need])
        # Lua Length 少 1：通常缺最后的 '}'
        consume = need
        obj: dict = {}
        type_digit = chr(payload[0]) if payload else ""

        def parse_payload(p: bytes) -> Optional[dict]:
            if len(p) < 2:
                return None
            try:
                return json.loads(p[1:].decode("ascii"))
            except (json.JSONDecodeError, UnicodeDecodeError):
                return None

        obj_try = parse_payload(payload)
        if obj_try is None:
            # 尝试多读 1~3 字节补全（覆盖 Length=实际-1 等）
            extended = False
            for extra in range(1, 4):
                if len(buffer) < need + extra:
                    break
                cand = bytes(buffer[3 : need + extra])
                if parse_payload(cand) is not None:
                    payload = cand
                    consume = need + extra
                    obj_try = parse_payload(cand)
                    extended = True
                    break
            if not extended and len(buffer) < need + 1:
                # 可能还有半包
                break

        if obj_try is not None:
            obj = obj_try
        type_digit = chr(payload[0]) if payload else type_digit

        raw = bytes(buffer[:3]) + payload
        del buffer[:consume]
        frames.append((raw, type_digit, obj))
    return frames


# ---------------------------------------------------------------------------
# 按 tset.lua 偏移构造回包（构造后用断言校验）
# ---------------------------------------------------------------------------

def _assert_lua_handshake(frame: bytes, device_id: int) -> None:
    status = to_hex_status(frame)
    assert status[52:56] == "6E61", f"handshake na mismatch: {status[52:56]}"
    id_str = str(device_id)
    if len(id_str) == 1:
        assert status[62:64] == f"{ord(id_str):02X}"
        assert status[64:66] == "22"
    else:
        assert status[62:64] == f"{ord(id_str[0]):02X}"
        assert status[64:66] == f"{ord(id_str[1]):02X}"


def _assert_lua_playing(frame: bytes, playing: bool) -> None:
    status = to_hex_status(frame)
    assert status[22:24] == "32", f"play marker mismatch: {status[22:24]}"
    expect = "30" if playing else "32"
    assert status[66:68] == expect, f"play flag mismatch: {status[66:68]} != {expect}"


def make_handshake_frame(device_id: int, *, lua_length: bool = True) -> bytes:
    """
    客户端握手。对应 tset.lua:
      string.sub(status,53,56)=='6E61'
      ID @ hex 63..
    Payload 设计保证: 字节27-28='na'，字节32起为 ID ASCII。
    """
    if device_id < 0 or device_id > 99:
        raise ValueError("device_id 需在 0~99")
    id_str = str(device_id)
    # com 用 "09" 两字符，才能让 "na" 落在 lua 检测的固定偏移上
    payload = f'2{{"tp":"1","com":"09","na":"{id_str}"}}'.encode("ascii")
    frame = encode_frame(payload, lua_length=lua_length)
    _assert_lua_handshake(frame, device_id)
    return frame


def make_playing_frame(playing: bool, *, lua_length: bool = True) -> bytes:
    """
    播放/停止状态。对应 tset.lua:
      播放: sub(23,24)=='32' and sub(67,68)=='30'
      停止: sub(23,24)=='32' and sub(67,68)=='32'
    即第 12 字节='2'，第 34 字节='0'/'2'。
    """
    flag = "0" if playing else "2"
    # "stat" 四字符键名，使 flag 落在第 34 字节
    payload = f'2{{"tp":"2","com":"10","stat":"{flag}"}}'.encode("ascii")
    frame = encode_frame(payload, lua_length=lua_length)
    _assert_lua_playing(frame, playing)
    return frame


def lua_classify(raw: bytes) -> str:
    """按 tset.lua 规则分类一帧（用于日志）。"""
    status = to_hex_status(raw)
    if len(status) >= 56 and status[52:56] == "6E61":
        return "handshake"
    if len(status) >= 68 and status[22:24] == "32":
        flag = status[66:68]
        if flag == "30":
            return "playing"
        if flag == "32":
            return "stopped"
    return "other"


class OmvClientSimulator:
    def __init__(
        self,
        host: str,
        port: int,
        device_id: int,
        auto_reply: bool = True,
        lua_length: bool = True,
    ):
        self.host = host
        self.port = port
        self.device_id = device_id
        self.auto_reply = auto_reply
        self.lua_length = lua_length
        self.sock: Optional[socket.socket] = None
        self.buffer = bytearray()
        self.playing = False
        self._stop = threading.Event()
        self._send_lock = threading.Lock()
        self._handshake_sent = False

    def connect(self) -> None:
        self.sock = socket.create_connection((self.host, self.port), timeout=10)
        self.sock.settimeout(0.5)
        print(
            f"[+] 已连接 {self.host}:{self.port}  device_id={self.device_id}"
            f"  lua_length={self.lua_length}"
        )
        # 连接后只发一次握手
        self.send_handshake()
        self._handshake_sent = True

    def close(self) -> None:
        self._stop.set()
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None

    def send_raw(self, frame: bytes, label: str = "") -> None:
        if not self.sock:
            print("[!] 未连接")
            return
        with self._send_lock:
            self.sock.sendall(frame)
        tag = f" ({label})" if label else ""
        print(f"[>] 发送{tag}: {hex_bytes(frame)}")
        print(f"    ASCII: {frame[3:].decode('ascii', errors='replace')}")
        print(f"    lua_class={lua_classify(frame)}  hex_status_len={len(to_hex_status(frame))}")

    def send_handshake(self) -> None:
        self.send_raw(
            make_handshake_frame(self.device_id, lua_length=self.lua_length),
            "握手",
        )

    def send_playing(self, playing: bool) -> None:
        self.playing = playing
        self.send_raw(
            make_playing_frame(playing, lua_length=self.lua_length),
            "正在播放" if playing else "已停止",
        )

    def handle_command(self, type_digit: str, obj: dict, raw: bytes) -> None:
        com = str(obj.get("com", ""))
        pm = str(obj.get("pm", ""))
        print(
            f"[<] 指令 type={type_digit!r} com={com!r} pm={pm!r} "
            f"lua_class={lua_classify(raw)} json={obj}"
        )

        if not self.auto_reply:
            return

        # 心跳 / 握手保活：忽略，不回握手
        if com in ("9", "09"):
            print("[*] 收到心跳，忽略")
            return

        # play → 正在播放
        if com == "10" and pm == "10":
            print("[*] 收到 play，回复正在播放")
            time.sleep(0.05)
            self.send_playing(True)
            return

        # stop → 已停止
        if com == "10" and pm == "17":
            print("[*] 收到 stop，回复已停止")
            time.sleep(0.05)
            self.send_playing(False)
            return

    def _recv_loop(self) -> None:
        assert self.sock is not None
        while not self._stop.is_set():
            try:
                data = self.sock.recv(4096)
            except socket.timeout:
                continue
            except OSError:
                break
            if not data:
                print("[!] 对端关闭连接")
                self._stop.set()
                break
            print(f"[<] 原始 hex: {hex_bytes(data)}")
            self.buffer.extend(data)
            for raw, type_digit, obj in try_decode(self.buffer):
                print(f"[<] 解帧: {hex_bytes(raw)}")
                print(f"    ASCII: {raw[3:].decode('ascii', errors='replace')}")
                self.handle_command(type_digit, obj, raw)

    def run_interactive(self) -> None:
        self.connect()
        t = threading.Thread(target=self._recv_loop, daemon=True)
        t.start()

        print(
            "\n命令: play | stop | hs(握手) | status | quit\n"
            "      也可直接粘贴十六进制（空格可选）发送原始帧\n"
        )
        try:
            while not self._stop.is_set():
                try:
                    line = input("> ").strip()
                except EOFError:
                    break
                if not line:
                    continue
                cmd = line.lower()
                if cmd in ("q", "quit", "exit"):
                    break
                if cmd == "play":
                    self.send_playing(True)
                elif cmd == "stop":
                    self.send_playing(False)
                elif cmd in ("hs", "handshake"):
                    self.send_handshake()
                elif cmd == "status":
                    print(
                        f"playing={self.playing} device_id={self.device_id} "
                        f"handshake_sent={self._handshake_sent}"
                    )
                elif all(c in "0123456789abcdefABCDEF " for c in line):
                    try:
                        raw = bytes.fromhex(line.replace(" ", ""))
                        self.send_raw(raw, "自定义 hex")
                    except ValueError as e:
                        print(f"[!] hex 无效: {e}")
                else:
                    print("[!] 未知命令")
        except KeyboardInterrupt:
            print("\n[!] 中断")
        finally:
            self.close()
            print("[*] 已断开")


def main() -> int:
    parser = argparse.ArgumentParser(description="OMV 客户端模拟器（遵循 tset.lua）")
    parser.add_argument("-H", "--host", default="127.0.0.1", help="控制端地址")
    parser.add_argument("-p", "--port", type=int, default=9090, help="控制端端口")
    parser.add_argument("-i", "--id", type=int, default=1, help="设备 ID (0~99)")
    parser.add_argument(
        "--no-auto",
        action="store_true",
        help="不自动响应 play/stop",
    )
    parser.add_argument(
        "--correct-len",
        action="store_true",
        help="Length 使用真实 payload 长度（默认按 lua 少写 1）",
    )
    args = parser.parse_args()

    client = OmvClientSimulator(
        host=args.host,
        port=args.port,
        device_id=args.id,
        auto_reply=not args.no_auto,
        lua_length=not args.correct_len,
    )
    try:
        client.run_interactive()
    except ConnectionRefusedError:
        print(f"[!] 无法连接 {args.host}:{args.port}，请先启动控制端 Server")
        return 1
    except OSError as e:
        print(f"[!] 连接失败: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
