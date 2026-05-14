import urllib.request
import json
import os
import ssl

ssl._create_default_https_context = ssl._create_unverified_context

folder = r'C:\Users\17537\Desktop\oc\数据库'

# Process the failed images with error handling
for img_file in ['第三次实验.png', '第七次实验.png', '第八次实验.png']:
    path = os.path.join(folder, img_file)
    
    with open(path, 'rb') as fh:
        img_data = fh.read()
    
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
            # Try to handle encoding issues
            print(f'=== {img_file} ===')
            # Escape non-ASCII for safe printing
            safe_text = parsed_text.encode('utf-8', errors='replace').decode('utf-8')
            print(safe_text[:2000])
            print()
    except Exception as e:
        print(f'=== {img_file} === Error: {e}')
        import traceback
        traceback.print_exc()
