import sys
sys.stdout.reconfigure(encoding='utf-8')

path = r'C:\Tangent\TRArchV7.0\tgKeyboardShortcuts.xml'

with open(path, 'rb') as f:
    data = f.read()

# Before: check what we added
count_empty = data.count(b'Shortcuts="" ')
print(f'Shortcuts="" occurrences before fix: {count_empty}')

# Fix: remove the Shortcuts="" we added
lines = data.split(b'\n')
fixed_count = 0
for i, line in enumerate(lines):
    if b'Shortcuts="" ' in line:
        lines[i] = line.replace(b'Shortcuts="" ', b'')
        fixed_count += 1

result = b'\n'.join(lines)
with open(path, 'wb') as f:
    f.write(result)

# Verify
verify_count = result.count(b'Shortcuts="" ')
print(f'Fixed {fixed_count} lines')
print(f'Shortcuts="" occurrences after fix: {verify_count}')
print('Done - tgKeyboardShortcuts.xml restored')
