#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ENTTEC S-Play HTTP API 模拟器（标准库，无第三方依赖）

用于在没有实物时测试 Flow 的 SPlay Controller 节点。
协议参考：https://github.com/ENTTEC/SplayApi

默认监听：http://127.0.0.1:8080/api
节点「主机」请填：127.0.0.1:8080
（节点支持 host 或 host:port；实物默认 :80 时只填 IP 即可）

用法：
  python mock_splay_server.py
  python mock_splay_server.py --port 8080 --host 0.0.0.0
  python mock_splay_server.py --count 5

自测：
  curl -H "Content-Type: application/json" -d "{\"command\":8}" http://127.0.0.1:8080/api
  curl -H "Content-Type: application/json" -d "{\"command\":0,\"playlist_id\":1}" http://127.0.0.1:8080/api
"""

from __future__ import annotations

import argparse
import json
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Dict, List
from urllib.parse import urlparse

# PLAYLIST_STATUS（与 SplayApi 一致）
STATUS_IDLE = 0
STATUS_PLAYING = 1
STATUS_PAUSED = 2
STATUS_STOPPED = 3
STATUS_STOPPING = 4
STATUS_ERROR = 5

STATUS_NAME = {
    STATUS_IDLE: "IDLE",
    STATUS_PLAYING: "PLAYING",
    STATUS_PAUSED: "PAUSED",
    STATUS_STOPPED: "STOPPED",
    STATUS_STOPPING: "STOPPING",
    STATUS_ERROR: "ERROR",
}


class SplayState:
    """内存中的 playlist 状态机。"""

    def __init__(self, count: int = 4) -> None:
        self._lock = threading.RLock()
        self.playlists: List[Dict[str, Any]] = []
        names = ["Lobby", "Show", "Ambient", "Finale", "Intermission", "Credits"]
        for i in range(count):
            pid = i + 1
            self.playlists.append(
                {
                    "playlist_id": pid,
                    "name": names[i] if i < len(names) else f"Playlist {pid}",
                    "order": i + 1,
                    "status": STATUS_STOPPED,
                    "current_time": 0.0,
                    "duration": 5000.0 + i * 1000.0,
                    "intensity": 100,
                    "hide_from_home": False,
                    "waiting_triggers": False,
                    "_started_at": None,  # monotonic，仅模拟器用
                }
            )

    def _tick_playing(self, pl: Dict[str, Any]) -> None:
        if pl["status"] != STATUS_PLAYING or pl["_started_at"] is None:
            return
        elapsed_ms = (time.monotonic() - pl["_started_at"]) * 1000.0
        duration = float(pl["duration"])
        # 简单循环：播完后从 0 继续
        pl["current_time"] = elapsed_ms % duration if duration > 0 else elapsed_ms

    def snapshot(self) -> Dict[str, Any]:
        with self._lock:
            out = []
            for pl in self.playlists:
                self._tick_playing(pl)
                item = {k: v for k, v in pl.items() if not k.startswith("_")}
                out.append(item)
            return {"playlists": out}

    def play(self, playlist_id: int) -> bool:
        with self._lock:
            pl = self._find(playlist_id)
            if pl is None:
                return False
            pl["status"] = STATUS_PLAYING
            pl["_started_at"] = time.monotonic()
            pl["current_time"] = 0.0
            return True

    def pause(self, playlist_id: int) -> bool:
        with self._lock:
            pl = self._find(playlist_id)
            if pl is None:
                return False
            self._tick_playing(pl)
            pl["status"] = STATUS_PAUSED
            pl["_started_at"] = None
            return True

    def stop(self, playlist_id: int) -> bool:
        with self._lock:
            pl = self._find(playlist_id)
            if pl is None:
                return False
            pl["status"] = STATUS_STOPPED
            pl["current_time"] = 0.0
            pl["_started_at"] = None
            return True

    def play_all(self) -> None:
        with self._lock:
            now = time.monotonic()
            for pl in self.playlists:
                pl["status"] = STATUS_PLAYING
                pl["_started_at"] = now
                pl["current_time"] = 0.0

    def stop_all(self) -> None:
        with self._lock:
            for pl in self.playlists:
                pl["status"] = STATUS_STOPPED
                pl["current_time"] = 0.0
                pl["_started_at"] = None

    def _find(self, playlist_id: int) -> Dict[str, Any] | None:
        for pl in self.playlists:
            if pl["playlist_id"] == playlist_id:
                return pl
        return None

    def summary(self) -> str:
        with self._lock:
            parts = []
            for pl in self.playlists:
                self._tick_playing(pl)
                st = STATUS_NAME.get(pl["status"], str(pl["status"]))
                parts.append(f"{pl['playlist_id']}:{pl['name']}={st}")
            return " | ".join(parts)


STATE = SplayState()


def handle_command(body: Dict[str, Any]) -> Dict[str, Any]:
    cmd = body.get("command")
    if cmd is None:
        return {"result": False, "error": "Missing command"}

    try:
        cmd = int(cmd)
    except (TypeError, ValueError):
        return {"result": False, "error": f"Invalid command: {cmd!r}"}

    # PLAY
    if cmd == 0:
        pid = int(body.get("playlist_id", -1))
        if not STATE.play(pid):
            return {"result": False, "error": f"Unknown playlist_id: {pid}"}
        return {"result": True}

    # PAUSE
    if cmd == 1:
        pid = int(body.get("playlist_id", -1))
        if not STATE.pause(pid):
            return {"result": False, "error": f"Unknown playlist_id: {pid}"}
        return {"result": True}

    # STOP
    if cmd == 2:
        pid = int(body.get("playlist_id", -1))
        if not STATE.stop(pid):
            return {"result": False, "error": f"Unknown playlist_id: {pid}"}
        return {"result": True}

    # PLAY_ALL
    if cmd == 5:
        STATE.play_all()
        return {"result": True}

    # STOP_ALL
    if cmd == 7:
        STATE.stop_all()
        return {"result": True}

    # GET_ALL_PLAYLISTS
    if cmd == 8:
        return STATE.snapshot()

    # GET_INFO
    if cmd == 88:
        return {
            "result": True,
            "info": {
                "product": "S-Play Mock",
                "version": "mock-1.0",
                "note": "Python simulator for SPlay Controller node",
            },
        }

    return {"result": False, "error": f"Unsupported command id: {cmd}"}


class SplayHandler(BaseHTTPRequestHandler):
    server_version = "SplayMock/1.0"

    def log_message(self, fmt: str, *args: Any) -> None:
        print(f"[{self.log_date_time_string()}] {self.address_string()} {fmt % args}")

    def _send_json(self, code: int, payload: Dict[str, Any]) -> None:
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_POST(self) -> None:
        path = urlparse(self.path).path.rstrip("/") or "/"
        if path != "/api":
            self._send_json(404, {"result": False, "error": f"Not found: {self.path}"})
            return

        length = int(self.headers.get("Content-Length", "0") or 0)
        raw = self.rfile.read(length) if length > 0 else b"{}"
        try:
            body = json.loads(raw.decode("utf-8") or "{}")
            if not isinstance(body, dict):
                raise ValueError("body must be a JSON object")
        except Exception as exc:  # noqa: BLE001
            self._send_json(400, {"result": False, "error": f"Invalid JSON: {exc}"})
            return

        cmd = body.get("command")
        print(f"  << command={cmd} body={body}")
        resp = handle_command(body)
        print(f"  >> {resp if cmd != 8 else '{playlists: N=%d}' % len(resp.get('playlists', []))}")
        print(f"  state: {STATE.summary()}")
        self._send_json(200, resp)

    def do_GET(self) -> None:
        # 方便浏览器探活
        path = urlparse(self.path).path.rstrip("/") or "/"
        if path in ("/", "/api"):
            self._send_json(
                200,
                {
                    "result": True,
                    "message": "S-Play mock is running. Use POST /api with JSON {\"command\":...}",
                    "playlists": STATE.snapshot()["playlists"],
                },
            )
            return
        self._send_json(404, {"result": False, "error": f"Not found: {self.path}"})


def main() -> None:
    parser = argparse.ArgumentParser(description="Mock ENTTEC S-Play HTTP API")
    parser.add_argument("--host", default="0.0.0.0", help="bind address (default 0.0.0.0)")
    parser.add_argument("--port", type=int, default=8080, help="listen port (default 8080)")
    parser.add_argument("--count", type=int, default=4, help="playlist count (default 4)")
    args = parser.parse_args()

    global STATE
    STATE = SplayState(count=max(1, args.count))

    httpd = ThreadingHTTPServer((args.host, args.port), SplayHandler)
    display_host = "127.0.0.1" if args.host in ("0.0.0.0", "::") else args.host
    print("=" * 60)
    print("S-Play Mock Server")
    print(f"  POST http://{display_host}:{args.port}/api")
    print(f"  Playlists: {args.count}")
    print()
    print("SPlay Controller 节点「主机」填写：")
    print(f"  {display_host}:{args.port}")
    print()
    print("试一下：")
    print(
        f'  curl -H "Content-Type: application/json" '
        f'-d "{{\\"command\\":8}}" http://{display_host}:{args.port}/api'
    )
    print("=" * 60)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nbye")
        httpd.server_close()


if __name__ == "__main__":
    main()
