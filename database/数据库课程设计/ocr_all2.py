import urllib.request
import json
import os
import ssl
import sys

ssl._create_default_https_context = ssl._create_unverified_context

folder = r'C:\Users\17537\Desktop\oc\数据库'
output_file = os.path.join(folder, 'ocr_results.txt')

results = []

# Process all images
for img_file in sorted(os.listdir(folder)):
    if not img_file.endswith('.png') or 'small' in img_file or 'tiny' in img_file or 'xsmall' in img_file or 'vsmall' in img_file or 'test' in img_file:
        continue
    
    path = os.path.join(folder, img_file)
    if not os.path.isfile(path):
        continue
    
    with open(path, 'rb') as fh:
        img_data = fh.read()
    
    result_entry = f'=== {img_file} ===\n'
    
    try:
        boundary = '----Boundary7MA4YWxkTrZu0gW'
        
        body = b''
        body += b'--' + boundary.encode() + b'\r\n'
        body += b'Content-Disposition: form-data; name="file"; filename="image.png"\r\n'
        body += b'Content-Type: image/png\r\n\r\n'
        body += img_data + b'\r\n'
        body += b'--' + boundary.encode() + b'\r\n'
        body += b'Content-Disposition: form-data; name="apikey"\r\n\r\n'
        body += b'helloworld\r\n'
        body += b'--' + boundary.encode() + b'\r\n'
        body += b'Content-Disposition: form-data; name="language"\r\n\r\n'
        body += b'chs\r\n'
        body += b'--' + boundary.encode() + b'--\r\n'
        
        req = urllib.request.Request(
            'https://api.ocr.space/parse/image',
            data=body,
            headers={'Content-Type': 'multipart/form-data; boundary=' + boundary}
        )
        
        with urllib.request.urlopen(req, timeout=30) as response:
            raw_bytes = response.read()
            result = json.loads(raw_bytes.decode('utf-8'))
            parsed_text = result.get('ParsedResults', [{}])[0].get('ParsedText', '')
            result_entry += parsed_text
    except Exception as e:
        result_entry += f'Error: {e}\n'
    
    results.append(result_entry)

# Write to file
with open(output_file, 'w', encoding='utf-8') as f:
    f.write('\n'.join(results))

print(f'Results written to {output_file}')
for r in results:
    print(r[:100] + '...')
