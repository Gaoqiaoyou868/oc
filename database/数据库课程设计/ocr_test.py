import requests
import os
from PIL import Image
import io

folder = r'C:\Users\17537\Desktop\oc\数据库'
f = '第一次实验.png'
path = os.path.join(folder, f)

# Try a different free OCR API
# Using OCR.space
try:
    with open(path, 'rb') as f:
        response = requests.post(
            'https://api.ocr.space/parse/image',
            files={'file': ('image.png', f, 'image/png')},
            data={'apikey': 'helloworld', 'language': 'chs'},
            timeout=30
        )
        print(response.json())
except Exception as e:
    print(f'OCR.space error: {e}')
