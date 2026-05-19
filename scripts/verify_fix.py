import sys, re
sys.stdout.reconfigure(encoding='utf-8')

path = r'C:\Users\17537\AppData\Roaming\Autodesk\Revit\Autodesk Revit 2018\KeyboardShortcuts.xml'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Show first 10 TR entries with Shortcuts populated
count = 0
for m in re.finditer(r'CommandId="(CustomCtrl_[^"]+)"[^>]*Shortcuts="([^"]+)"', content):
    cmd = m.group(1)
    sht = m.group(2)
    print(f'{cmd[:60]} -> {sht}')
    count += 1
    if count >= 10:
        break

# Count totals
total_filled = len(re.findall(r'CustomCtrl_%[^"]*" Shortcuts="', content))
total_all = len(re.findall(r'CustomCtrl_%[^"]*" Paths=', content))
total_missing = len(re.findall(r'CustomCtrl_%[^"]*" Paths=', content)) - total_filled

print(f'\nTotal TR entries: {total_all}')
print(f'With Shortcuts: {total_filled}')
print(f'Still missing: {total_missing}')

# Verify file integrity
if content.count('<Shortcuts>') == 1 and content.count('</Shortcuts>') == 1:
    print('\nXML structure intact: <Shortcuts>...</Shortcuts> OK')
