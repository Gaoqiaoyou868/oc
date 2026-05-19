import sys, re
sys.stdout.reconfigure(encoding='utf-8')

path = r'C:\Users\17537\AppData\Roaming\Autodesk\Revit\Autodesk Revit 2018\KeyboardShortcuts.xml'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Correct count using proper regex: look for Shortcuts= after a TR CommandId
total_tr = len(re.findall(r'CommandId="(?:CustomCtrl_%|ID_)', content))
filled = len(re.findall(r'CommandId="CustomCtrl_%[^"]*"[\s\S]*?Shortcuts=', content))
native = len(re.findall(r'CommandId="ID_[^"]*"[\s\S]*?Shortcuts=', content))

print(f'=== Verification Report ===')
print(f'Total commands in file: {len(re.findall(r"<ShortcutItem", content))}')
print(f'TR commands (CustomCtrl_%): {len(re.findall(r"CommandId=\"CustomCtrl_%", content))}')
print(f'  With Shortcuts filled: {filled}')
print(f'  Still missing: {len(re.findall(r"CommandId=\"CustomCtrl_%", content)) - filled}')
print(f'Native Revit commands (ID_): {len(re.findall(r"CommandId=\"ID_", content))}')
print(f'  With Shortcuts: {native}')

# 2. XML integrity check
opens = len(re.findall(r'<ShortcutItem', content))
closes = len(re.findall(r'/>', content))
print(f'\nXML integrity:')
print(f'  <ShortcutItem tags: {opens}')
print(f'  Self-closing /> : {closes}')

if opens == closes:
    print('  OK: All tags properly closed')
else:
    diff = abs(opens - closes)
    print(f'WARNING: {diff} unclosed tags')

# 3. Check the 2 TR source entries that had no Shortcuts (should NOT be filled)
no_shortcuts_src = [
    'CustomCtrl_%CustomCtrl_%CustomCtrl_%天正建筑%其他工具%数据 中心%审查数据',
    'CustomCtrl_%CustomCtrl_%CustomCtrl_%天正工具%其他工具%关于%导入天正快捷键'
]
for cmd in no_shortcuts_src:
    if cmd in content:
        idx = content.find(cmd)
        chunk = content[idx:idx+200]
        if 'Shortcuts=' in chunk:
            print(f'WARNING: Entry without source Shortcuts was wrongly filled: {cmd[:60]}')
        else:
            print(f'OK: Entry correctly left blank: {cmd[:60]}...')

# 4. Show sample with shortcuts
print(f'\n=== Sample entries ===')
for m in re.finditer(r'CommandId="(CustomCtrl_%[^"]+)"[\s\S]{0,50}Shortcuts="([^"]+)"', content):
    print(f'  {m.group(1)[:55]} -> {m.group(2)}')
    if m.group(2) == 'TRFL':
        print(f'    ^ This was the ONLY entry with Shortcuts before fix')
        break

print(f'\n=== File size ===')
import os
size = os.path.getsize(path)
print(f'{size} bytes')
