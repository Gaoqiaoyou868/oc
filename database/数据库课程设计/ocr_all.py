import urllib.request
import json
import os
import ssl

ssl._create_default_https_context = ssl._create_unverified_context

folder = r'C:\Users\17537\Desktop\oc\数据库'

# Process all 8 images
for img_file in ['第一次实验.png', '第二次实验.png', '第三次实验.png', '第四次实验.png', '第五次实验.png', '第六次实验.png', '第七次实验.png', '第八次实验.png']:
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
            result = json.loads(response.read().decode('utf-8'))
            parsed_text = result.get('ParsedResults', [{}])[0].get('ParsedText', '')
            print(f'=== {img_file} ===')
            print(parsed_text[:2000])
            print()
    except Exception as e:
        print(f'=== {img_file} === Error: {e}')
