import os, sys
sys.stdout.reconfigure(encoding='utf-8')

fpath = r'C:\Tangent\TRArchV7.0\TGstart.exe'
with open(fpath, 'rb') as f:
    data = f.read()

text = data.decode('utf-16-le', errors='replace')

keywords = ['Revit', 'revit', 'EXE', 'Start', 'Launch', 'Config', 'TGstart', 'Regist', 'reg', 'Addin', 'addin', 'Command', 'Path', 'param', 'Param', '.exe', '2018']
seen = set()
for kw in keywords:
    idx = 0
    while True:
        idx = text.find(kw, idx)
        if idx < 0:
            break
        start = max(0, idx - 80)
        end = min(len(text), idx + len(kw) + 150)
        ctx = text[start:end]
        ctx_clean = ''.join(c if c.isprintable() else ' ' for c in ctx)
        if ctx_clean not in seen:
            seen.add(ctx_clean)
            print(f'Found "{kw}":')
            print(f'  {ctx_clean}')
            print()
        idx += 1
