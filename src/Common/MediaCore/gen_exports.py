#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MediaCore 自动导出符号表生成器（DLL def）。

背景：CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 生成的 exports.def 会把
guiddef.h（extern "C" 块内的 __inline GUID operator==/!=）产生的
字面名符号 `==` / `!=` 写进 EXPORTS，link.exe 解析时 LNK2001。
（MediaServer 静态库时代不生成 def，从未暴露；全量 DLL 化首次触发。）

本脚本复刻 CMake 的 def 生成逻辑（dumpbin /SYMBOLS → External 段符号），
并过滤非法符号名（必须以 ?、字母或下划线开头），输出标准 .def 文件。

用法：python gen_exports.py <obj_dir> <dumpbin.exe> <output.def>
  obj_dir   —— MediaCore.dir 目标目录（递归扫 *.obj）
  dumpbin   —— 与 cl 同目录的 dumpbin.exe
  output    —— def 输出路径

增量策略：def 已存在且新于全部 obj 时跳过（MSVC 慢启动 + 47 obj 全量
dumpbin 约 2~5s，每次链接重跑代价可接受，故只做存在性+时间戳粗判）。
"""
import glob
import os
import re
import subprocess
import sys

# dumpbin 符号行：`8D7 00000000 SECT1C7 notype ()    External    | ==`
SYM_RE = re.compile(
    r"^[0-9A-Fa-f]{3,8}\s+[0-9A-Fa-f]{8}\s+(SECT\w+)\s+"
    r"(notype\s*\(\)|notype)\s+(External|WeakExternal)\s*\|\s*(.+?)\s*$")
# 合法导出名：mangled(?开头) 或 C 名（字母/下划线开头）——排除 == != 等
VALID_NAME_RE = re.compile(r"^[?A-Za-z_][^|]*$")


def main():
    obj_dir, dumpbin, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    objs = glob.glob(os.path.join(obj_dir, "**", "*.obj"), recursive=True)
    if not objs:
        print("gen_exports: no obj found in", obj_dir, file=sys.stderr)
        sys.exit(1)

    # 粗增量：def 比全部 obj 新则跳过
    if os.path.exists(out_path):
        out_mtime = os.path.getmtime(out_path)
        if all(os.path.getmtime(o) <= out_mtime for o in objs):
            return

    funcs, datas = set(), set()
    for o in objs:
        out = subprocess.check_output([dumpbin, "/SYMBOLS", o],
                                      text=True, errors="ignore")
        for line in out.splitlines():
            m = SYM_RE.match(line.strip())
            if not m:
                continue
            sect, kind, linkage, sym = m.groups()
            if linkage == "WeakExternal":
                continue
            # dumpbin 在 | 后给 mangled 名 + 括号 demangle 注释，取第一段
            name = sym.split("(", 1)[0].strip()
            if not VALID_NAME_RE.match(name):
                continue
            if name.startswith("__"):   # 编译器内部符号（__real@/__xt_ 等）
                continue
            if kind == "notype ()":
                funcs.add(name)
            else:
                datas.add(name)

    with open(out_path, "w", encoding="ascii") as f:
        f.write("EXPORTS\n")
        for n in sorted(funcs):
            f.write("    %s\n" % n)
        for n in sorted(datas):
            f.write("    %s DATA\n" % n)
    print("gen_exports: %d funcs / %d datas -> %s"
          % (len(funcs), len(datas), out_path))


if __name__ == "__main__":
    main()
