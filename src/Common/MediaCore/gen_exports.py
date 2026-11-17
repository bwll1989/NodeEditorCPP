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

扫描白名单（2026-11-17）：给 build.ninja + dll_name 时从 MediaCore 的
链接行提取 obj 清单（=链接器真实输入）。此前用 glob("**/*.obj") 扫整个
MediaCore.dir，源文件删除后 Ninja 残留孤儿 obj（net/Discovery.cpp.obj
实证）会被误扫进 def——其符号不在链接输入 → MediaCore.exp LNK2001
（std::thread/_Compressed_pair 模板实例构造符号实证）。白名单模式天然
排除孤儿；无参时退回目录扫描（兼容旧调用）。

用法：python gen_exports.py <obj_dir> <dumpbin> <output.def> [build_ninja] [dll_name]
  obj_dir     —— fallback 目录（build_ninja 缺失时递归扫 *.obj）
  dumpbin     —— 与 cl 同目录的 dumpbin.exe
  output      —— def 输出路径
  build_ninja —— build/build.ninja（Ninja 生成器产物）
  dll_name    —— 目标 DLL 输出名，如 MediaCore.dll

增量策略：def 已存在且新于全部（白名单内）obj 时跳过（MSVC 慢启动 +
dumpbin 全量约 2~5s，每次链接重跑代价可接受，故只做时间戳粗判）。
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


def objs_from_build_ninja(ninja_path, dll_name):
    """从 build.ninja 提取 dll_name 链接行的全部 obj 输入（绝对路径列表）。

    build 行格式：`build <outputs>: <rule> <explicit_inputs> | <implicit> || <order>`
    Ninja 转义：`$:` → `:`（盘符）、`$ ` → 空格、行尾 `$` 续行。
    解析失败返回 None，调用方退回目录扫描。
    """
    try:
        with open(ninja_path, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
    except OSError:
        return None
    text = text.replace("$\n", " ")            # 续行拼接
    for line in text.splitlines():
        if not line.startswith("build "):
            continue
        head, colon, rest = line.partition(":")
        outputs = head[len("build "):].split()
        if not any(os.path.basename(o.replace("$:", ":")) == dll_name
                   for o in outputs):
            continue
        rule, _, inputs = rest.partition(" ")
        explicit = inputs.split("|", 1)[0]
        objs = []
        for tok in explicit.split():
            t = tok.replace("$:", ":").replace("$ ", " ")
            if t.endswith(".obj"):
                objs.append(t if os.path.isabs(t)
                            else os.path.join(os.path.dirname(ninja_path), t))
        return objs
    return None


def collect_symbols(objs, dumpbin):
    """dumpbin /SYMBOLS 扫 obj → (funcs, datas) 集合。"""
    funcs, datas = set(), set()
    for o in objs:
        out = subprocess.check_output([dumpbin, "/SYMBOLS", o],
                                      text=True, errors="ignore")
        for line in out.splitlines():
            m = SYM_RE.match(line.strip())
            if not m:
                continue
            _sect, kind, linkage, sym = m.groups()
            if linkage == "WeakExternal":
                continue
            # vtordisp/adjustor thunk：def 导出触发 LNK4102 警告（映射名
            # 可能不正确），且消费方不应依赖 thunk——直接排除
            if "[thunk]" in sym:
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
    return funcs, datas


def main():
    argv = sys.argv[1:]
    if len(argv) < 3:
        print("usage: gen_exports.py <obj_dir> <dumpbin> <out.def> "
              "[build_ninja] [dll_name]", file=sys.stderr)
        sys.exit(2)
    obj_dir, dumpbin, out_path = argv[0], argv[1], argv[2]
    ninja_path = argv[3] if len(argv) > 3 else None
    dll_name = argv[4] if len(argv) > 4 else None

    objs = None
    if ninja_path and dll_name:
        objs = objs_from_build_ninja(ninja_path, dll_name)
        if objs is None:
            print("gen_exports: WARN %s 中未找到 %s 链接行，退回目录扫描"
                  % (ninja_path, dll_name), file=sys.stderr)
    if not objs:
        objs = glob.glob(os.path.join(obj_dir, "**", "*.obj"), recursive=True)
    if not objs:
        print("gen_exports: no obj found for", obj_dir, file=sys.stderr)
        sys.exit(1)

    # 粗增量：def 比全部（白名单内）obj 新则跳过
    if os.path.exists(out_path):
        out_mtime = os.path.getmtime(out_path)
        if all(os.path.getmtime(o) <= out_mtime for o in objs):
            return

    funcs, datas = collect_symbols(objs, dumpbin)

    with open(out_path, "w", encoding="ascii") as f:
        f.write("EXPORTS\n")
        for n in sorted(funcs):
            f.write("    %s\n" % n)
        for n in sorted(datas):
            f.write("    %s DATA\n" % n)
    print("gen_exports: %d funcs / %d datas (%d obj) -> %s"
          % (len(funcs), len(datas), len(objs), out_path))


if __name__ == "__main__":
    main()
