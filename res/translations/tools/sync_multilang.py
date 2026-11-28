# -*- coding: utf-8 -*-
"""把 flow_en.ts 的词条结构同步到快照语言 .ts（ja/zh_TW/fr）。

- flow_en.ts 是 lupdate 唯一维护对象；快照语言按 (context, source) 对齐
- 新增词条：插入译文（zh_TW=s2twp 转换；ja/fr 查可选映射表，缺则空译回退中文）
- flow_en 已删词条：从快照同步删除
- 用法：python Temp/sync_multilang.py [lang ...]  默认 ja zh_TW fr
"""
import sys, json, xml.etree.ElementTree as ET
from opencc import OpenCC

CC = OpenCC('s2twp')
# 可选手工映射：(lang, ctx, src) -> 译文
EXTRA = {
    ('en', '*', '*'): {},
    ('ja', 'SettingWidget', '简体中文'): '簡体字中国語',
    ('ja', 'SettingWidget', '繁体中文'): '繁体字中国語',
    ('fr', 'SettingWidget', '简体中文'): 'Chinois (simplifié)',
    ('fr', 'SettingWidget', '繁体中文'): 'Chinois (traditionnel)',
    ('ja', 'SettingWidget', '浅色'): 'ライト',
    ('ja', 'SettingWidget', '深色'): 'ダーク',
    ('ja', 'SettingWidget', '界面主题，切换后重启生效'): 'テーマは再起動後に反映されます',
    ('fr', 'SettingWidget', '浅色'): 'Clair',
    ('fr', 'SettingWidget', '深色'): 'Sombre',
    ('fr', 'SettingWidget', '界面主题，切换后重启生效'): "Thème de l'interface, appliqué après redémarrage",
}

def lookup(lang, ctx, src):
    for (l, c, s), t in EXTRA.items():
        if l == lang and (c == '*' or c == ctx) and (s == '*' or s == src):
            return t
    return None

def snapshot(lang):
    en = ET.parse('res/translations/flow_en.ts').getroot()
    p = 'res/translations/flow_%s.ts' % lang
    tree = ET.parse(p)
    root = tree.getroot()

    # flow_en 现行词条集合（保序）
    en_ctxs = []
    for ctx in en.iter('context'):
        cname = ctx.find('name').text
        srcs = [m.find('source').text for m in ctx.iter('message') if m.find('source') is not None]
        en_ctxs.append((cname, srcs))

    added, removed, refilled = 0, 0, 0
    for cname, srcs in en_ctxs:
        tctx = None
        for ctx in root.iter('context'):
            if ctx.find('name').text == cname:
                tctx = ctx
                break
        if tctx is None:
            tctx = ET.Element('context')
            ET.SubElement(tctx, 'name').text = cname
            root.append(tctx)
        have = set()
        for m in list(tctx.findall('message')):
            s = m.find('source')
            if s is None or s.text not in srcs:
                tctx.remove(m)  # flow_en 已删，快照同步删
                removed += 1
            else:
                have.add(s.text)
        for src in srcs:
            if src in have:
                continue
            m = ET.SubElement(tctx, 'message')
            ET.SubElement(m, 'source').text = src
            tr = ET.SubElement(m, 'translation')
            t = lookup(lang, cname, src)
            if t is None and lang == 'zh_TW':
                t = CC.convert(src)
            if t is not None:
                tr.text = t
            else:
                tr.set('type', 'unfinished')
            added += 1
        # 回填：已存在但仍是 unfinished 的词条（上轮无映射留下的洞）
        for m in tctx.findall('message'):
            tr = m.find('translation')
            if tr is None or tr.get('type') != 'unfinished':
                continue
            src = m.find('source').text
            t = lookup(lang, cname, src)
            if t is None and lang == 'zh_TW':
                t = CC.convert(src)
            if t is not None:
                tr.text = t
                del tr.attrib['type']
                refilled += 1

    # 空 context 清理
    for ctx in list(root.iter('context')):
        if len(ctx.findall('message')) == 0:
            root.remove(ctx)
    tree.write(p, encoding='utf-8', xml_declaration=True)
    total = sum(len(c.findall('message')) for c in root.iter('context'))
    unfin = sum(1 for c in root.iter('context') for m in c.iter('message')
                if m.find('translation') is not None and m.find('translation').get('type') == 'unfinished')
    print('[%s] added=%d removed=%d refilled=%d total=%d unfinished=%d' % (lang, added, removed, refilled, total, unfin))

for lang in (sys.argv[1:] or ['ja', 'zh_TW', 'fr']):
    snapshot(lang)
