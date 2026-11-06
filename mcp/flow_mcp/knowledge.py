# -*- coding: utf-8 -*-
"""节点知识库检索：加载 docs/ai-knowledge.json，供 MCP 工具按需查询。

知识库结构：
  keys:   {别名/变体名: 规范注册名, ...}   —— 361 条，解决 "AudioOut"→"Audio Out"
  nodes:  {规范注册名: {tag, summary, ports:{in:[],out:[]}, props:[...]}}
"""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any, Optional


def _default_knowledge_path() -> Path:
    """知识库查找顺序（第一个存在的生效）：
    1. 环境变量 FLOW_KNOWLEDGE_PATH
    2. 开发布局：<项目根>/docs/ai-knowledge.json（源码树内，文档流水线更新——优先，永远最新）
    3. 分发布局：flow_mcp/data/ai-knowledge.json（独立分发 mcp/ 文件夹时随包自带）
    """
    env = os.environ.get("FLOW_KNOWLEDGE_PATH")
    if env:
        return Path(env)
    here = Path(__file__).resolve()
    candidates = [
        here.parents[2] / "docs" / "ai-knowledge.json",       # 开发布局（优先：永远最新）
        here.parent / "data" / "ai-knowledge.json",           # 独立分发（自包含副本）
    ]
    for p in candidates:
        if p.is_file():
            return p
    return candidates[1]  # 都不存在时返回发布副本位置，让上层抛出带路径的明确错误


class Knowledge:
    def __init__(self, path: Optional[Path] = None) -> None:
        self.path = Path(path) if path else _default_knowledge_path()
        if not self.path.is_file():
            raise FileNotFoundError(f"知识库不存在: {self.path}")
        data = json.loads(self.path.read_text(encoding="utf-8"))
        self.keys: dict[str, str] = data.get("keys", {})
        self.nodes: dict[str, dict] = data.get("nodes", {})
        # 反向索引：小写压缩名 → 规范名（用于模糊归一化）
        self._compact = {
            k.replace(" ", "").lower(): v for k, v in self.keys.items()
        }

    # ------------------------------------------------------------------
    def normalize(self, name: str) -> Optional[str]:
        """把任意写法（含无空格别名）归一化为规范注册名。"""
        if name in self.nodes:
            return name
        if name in self.keys:
            return self.keys[name]
        hit = self._compact.get(name.replace(" ", "").lower())
        if hit:
            return hit
        return None

    def get(self, name: str) -> Optional[dict]:
        """取单节点文档（自动别名归一化）。"""
        norm = self.normalize(name)
        return self.nodes.get(norm) if norm else None

    # ------------------------------------------------------------------
    def search(self, keyword: str, limit: int = 8) -> list[dict]:
        """按关键词检索节点：匹配名称/别名/分类/摘要/端口名/属性名。"""
        kw = keyword.strip().lower()
        if not kw:
            return []
        scored: list[tuple[int, str]] = []
        for name, doc in self.nodes.items():
            score = 0
            if kw == name.lower():
                score = 100
            elif kw in name.lower():
                score = 80
            elif kw in str(doc.get("tag", "")).lower():
                score = 40
            elif kw in str(doc.get("summary", "")).lower():
                score = 20
            else:
                ports = doc.get("ports", {})
                port_text = json.dumps(
                    ports.get("in", []) + ports.get("out", []), ensure_ascii=False
                )
                props_text = json.dumps(doc.get("props", []), ensure_ascii=False)
                if kw in port_text.lower():
                    score = 15
                elif kw in props_text.lower():
                    score = 10
            if score:
                scored.append((score, name))
        scored.sort(key=lambda x: (-x[0], x[1]))
        results = []
        for _, name in scored[:limit]:
            doc = self.nodes[name]
            results.append(
                {
                    "type": name,
                    "tag": doc.get("tag", ""),
                    "summary": doc.get("summary", ""),
                    "ports": doc.get("ports", {}),
                    "props": doc.get("props", []),
                }
            )
        return results

    def all_names(self) -> list[str]:
        """全部规范注册名（附分类）。"""
        return [
            f"{name}  [{doc.get('tag', 'Other')}]" for name, doc in self.nodes.items()
        ]
