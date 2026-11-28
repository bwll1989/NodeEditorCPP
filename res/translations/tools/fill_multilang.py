# -*- coding: utf-8 -*-
"""把代理翻译 JSON (c/s/t) 填入对应 flow_<lang>.ts，并校验覆盖度。"""
import json, sys, xml.etree.ElementTree as ET

def fill(lang):
    terms = json.load(open('Temp/i18n_%s_terms.json' % lang, encoding='utf-8'))
    assert len(terms) == 2266, 'terms count %d != 2266' % len(terms)
    # 索引：按顺序对齐（代理保证顺序一致），同时用 (c,s) 双保险校验
    tree = ET.parse('res/translations/flow_%s.ts' % lang)
    root = tree.getroot()
    msgs = []
    for ctx in root.iter('context'):
        cname = ctx.find('name').text
        for msg in ctx.iter('message'):
            src = msg.find('source')
            tr = msg.find('translation')
            if src is None or tr is None:
                continue
            msgs.append((cname, src.text, tr))
    assert len(msgs) == 2266, 'ts count %d != 2266' % len(msgs)

    by_key = {}
    for i, t in enumerate(terms):
        by_key[(t['c'], t['s'])] = (i, t['t'])

    missing, mismatch, filled = [], [], 0
    for i, (cname, src, tr) in enumerate(msgs):
        key = (cname, src)
        if key not in by_key:
            missing.append(key)
            continue
        idx, translated = by_key[key]
        if idx != i:
            mismatch.append((i, idx, key))
        if not translated or not translated.strip():
            missing.append(('EMPTY', key))
            continue
        tr.text = translated
        if 'type' in tr.attrib:
            del tr.attrib['type']
        filled += 1

    print('[%s] filled=%d missing=%d order-mismatch=%d' % (lang, filled, len(missing), len(mismatch)))
    if missing:
        for m in missing[:10]:
            print('  MISS:', m)
        sys.exit(1)
    if mismatch:
        print('  order drift (still filled by (c,s) key):', mismatch[:5])
    tree.write('res/translations/flow_%s.ts' % lang, encoding='utf-8', xml_declaration=True)
    print('[%s] written' % lang)

for lang in sys.argv[1:]:
    fill(lang)
