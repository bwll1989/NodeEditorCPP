# -*- coding: utf-8 -*-
"""flow-mcp 自包含启动器。

不依赖工作目录（cwd），供 MCP 客户端以绝对路径直接拉起：
    python D:/19_NodeEditorCPP/NodeEditorCPP/mcp/run_server.py
"""

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from flow_mcp.server import main  # noqa: E402

if __name__ == "__main__":
    main()
