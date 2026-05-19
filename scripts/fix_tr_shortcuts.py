import sys, os, re

sys.stdout.reconfigure(encoding='utf-8')

# --- Step 1: Parse TR source file ---
src_path = r'C:\Tangent\TRArchV7.0\tgKeyboardShortcuts.xml'
with open(src_path, 'rb') as f:
    src_raw = f.read()

# Try GBK for TR source
for enc in ['gbk', 'gb2312', 'gb18030', 'utf-8']:
    try:
        src_text = src_raw.decode(enc)
        break
    except:
        src_text = src_raw.decode('utf-8', errors='replace')

src_shortcuts = {}
for m in re.finditer(r'<ShortcutItem[^>]*CommandId="([^"]+)"[^>]*Shortcuts="([^"]+)"[^>]*/>', src_text):
    cmd_id = m.group(1)
    shortcuts = m.group(2)
    src_shortcuts[cmd_id] = shortcuts

print(f'TR source: {len(src_shortcuts)} entries with Shortcuts defined')

# --- Step 2: Parse Revit KeyboardShortcuts.xml ---
revit_path = os.path.expandvars(r'%APPDATA%\Autodesk\Revit\Autodesk Revit 2018\KeyboardShortcuts.xml')
backup_path = os.path.join(os.path.dirname(__file__), 'backup', 'KeyboardShortcuts_backup.xml')

with open(revit_path, 'rb') as f:
    revit_raw = f.read()

# Try UTF-8 for Revit file
revit_text = revit_raw.decode('utf-8', errors='replace')

# Find all TR ShortcutItem elements
revit_tr = {}
for m in re.finditer(r'(<ShortcutItem[^>]*CommandId="(CustomCtrl_[^"]+)"[^>]*)/>', revit_text):
    full_tag = m.group(1)
    cmd_id = m.group(2)
    has_shortcuts = 'Shortcuts=' in full_tag
    revit_tr[cmd_id] = {'full_tag': full_tag, 'has_shortcuts': has_shortcuts}

tr_without = [k for k, v in revit_tr.items() if not v['has_shortcuts']]
tr_with = [k for k, v in revit_tr.items() if v['has_shortcuts']]
print(f'Revit TR entries: {len(revit_tr)} total')
print(f'  {len(tr_with)} already have Shortcuts')
print(f'  {len(tr_without)} missing Shortcuts')

# --- Step 3: Match and prepare replacements ---
matched = 0
replacements = []
for cmd_id in tr_without:
    if cmd_id in src_shortcuts:
        shortcuts = src_shortcuts[cmd_id]
        old_tag = revit_tr[cmd_id]['full_tag']
        new_tag = old_tag + ' Shortcuts="' + shortcuts + '"'
        replacements.append((old_tag + '/>', new_tag + '/>'))
        matched += 1

print(f'\nCan fix {matched} out of {len(tr_without)} missing entries')

# Show what we're about to do
for old, new in replacements[:3]:
    print(f'\n  OLD: {old[:100]}...')
    print(f'  NEW: {new[:100]}...')

# --- Step 4: Apply replacements ---
if matched == 0:
    print('\nNothing to fix. Exiting.')
    sys.exit(0)

new_content = revit_text
for old, new in replacements:
    if old in new_content:
        new_content = new_content.replace(old, new, 1)
    else:
        print(f'WARNING: Could not find: {old[:80]}...')

# --- Step 5: Write backup and save ---
# Backup already done earlier, write updated file
with open(revit_path, 'w', encoding='utf-8') as f:
    f.write(new_content)

# Verify
verify_tr = len(re.findall(r'CustomCtrl_%', new_content))
verify_shortcuts = len(re.findall(r'CustomCtrl_%[^"]*" Shortcuts="', new_content))
print(f'\nVerification:')
print(f'  Total TR references: {verify_tr}')
print(f'  TR entries with Shortcuts: {verify_shortcuts}')
print(f'\nDone! Updated file written to: {revit_path}')
