# -*- coding: utf-8 -*-
"""Flow HTTP 客户端：封装 Flow/FlowRuntime 的 8992 端口 HTTP API。

仅用标准库（urllib），零第三方依赖。
"""

from __future__ import annotations

import json
import os
import urllib.error
import urllib.request
from typing import Any, Optional


class FlowError(Exception):
    """Flow API 调用失败。"""


class FlowClient:
    """与 Flow（GUI）或 FlowRuntime（Headless）的 HTTP 服务通信。"""

    def __init__(
        self,
        host: Optional[str] = None,
        port: Optional[int] = None,
        timeout: float = 30.0,
    ) -> None:
        self.host = host or os.environ.get("FLOW_HOST", "127.0.0.1")
        self.port = int(port or os.environ.get("FLOW_PORT", "8992"))
        self.timeout = timeout
        self.base = f"http://{self.host}:{self.port}"
        # 关键：禁用系统代理，确保对 127.0.0.1 的请求不被代理软件劫持（否则返回 502）
        self._opener = urllib.request.build_opener(
            urllib.request.ProxyHandler({}),
        )

    # ------------------------------------------------------------------
    def _request(
        self,
        method: str,
        path: str,
        data: Optional[bytes] = None,
        content_type: str = "application/json; charset=utf-8",
    ) -> tuple[int, bytes]:
        req = urllib.request.Request(
            self.base + path,
            data=data,
            method=method,
            headers={"Content-Type": content_type}
            if data is not None
            else {},
        )
        try:
            with self._opener.open(req, timeout=self.timeout) as resp:
                return resp.status, resp.read()
        except urllib.error.HTTPError as exc:  # 4xx/5xx 也带回包体
            return exc.code, exc.read()
        except urllib.error.URLError as exc:
            raise FlowError(
                f"无法连接 Flow ({self.base})：{exc.reason}。"
                "请确认 Flow/FlowRuntime 正在运行，或用环境变量 FLOW_HOST/FLOW_PORT 指定地址。"
            ) from exc

    @staticmethod
    def _decode(body: bytes) -> Any:
        try:
            return json.loads(body.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return body.decode("utf-8", errors="replace")

    # ------------------------------------------------------------------
    def get_app_info(self) -> dict:
        """GET /api/info/app — 软件名与版本（兼探活）。"""
        status, body = self._request("GET", "/api/info/app")
        if status != 200:
            raise FlowError(f"/api/info/app 返回 {status}")
        return json.loads(body.decode("utf-8"))

    def get_current_flow_info(self) -> dict:
        """GET /api/info/current_flow — 当前工程元信息（可能 ok=false）。"""
        status, body = self._request("GET", "/api/info/current_flow")
        return json.loads(body.decode("utf-8"))

    def download_current_flow(self) -> Optional[bytes]:
        """GET /api/download/current_flow — 下载当前工程 .flow 字节。

        无工程 / 文件不存在时返回 None。
        """
        status, body = self._request("GET", "/api/download/current_flow")
        if status == 404:
            return None
        if status != 200:
            raise FlowError(f"下载当前工程失败：HTTP {status}")
        return body

    def upload_flow(self, filename: str, data: bytes) -> dict:
        """POST /api/upload/flow?filename=… — 上传 .flow 并触发加载。

        成功后 Flow 主窗口就地加载该工程（flowFileUploaded → loadFileFromPath）。
        """
        from urllib.parse import quote

        path = f"/api/upload/flow?filename={quote(filename)}"
        status, body = self._request(
            "POST", path, data=data, content_type="application/octet-stream"
        )
        result = json.loads(body.decode("utf-8"))
        if status != 200 or not result.get("ok"):
            raise FlowError(f"上传失败：HTTP {status} {result}")
        return result

    def send_command(self, address: str, value: Any) -> dict:
        """POST /api/command — 发送控制指令（OSC 地址树）。"""
        payload = json.dumps({"address": address, "value": value}).encode("utf-8")
        status, body = self._request("POST", "/api/command", data=payload)
        result = json.loads(body.decode("utf-8"))
        if status != 200 or not result.get("ok"):
            raise FlowError(f"指令被拒绝：HTTP {status} {result}")
        return result
