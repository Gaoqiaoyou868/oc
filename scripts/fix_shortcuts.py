import sys

filepath = r'C:\Tangent\TRArchV7.0\tgKeyboardShortcuts.xml'
with open(filepath, 'rb') as f:
    data = f.read()

lines = data.split(b'\n')
count = 0
for i, line in enumerate(lines):
    if b'ShortcutItem' in line and b'Shortcuts=' not in line:
        new_line = line.replace(b'Paths=', b'Shortcuts="" Paths=')
        lines[i] = new_line
        count += 1
        print(f'Fixed line {i+1}')

print(f'Fixed {count} entries')
result = b'\n'.join(lines)
with open(filepath, 'wb') as f:
    f.write(result)
print('Done')
