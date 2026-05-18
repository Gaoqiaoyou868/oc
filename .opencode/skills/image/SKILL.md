---
name: image
description: "Analyze and describe image files using Python (Pillow + OpenCV + EasyOCR). Use when the user provides an image file path, screenshot, or asks 'what is in this image', '帮我看看这张图', '读取图片', or references an image file (.png, .jpg, .jpeg, .bmp, .gif, .webp)."
---

# Image Analysis Skill

## When to use
Use this skill when the user gives you an image file path, pastes a screenshot, or asks you to look at an image. The model itself cannot see images, so use the Python script to extract information from the image file.

## Workflow
1. When user references an image file (e.g. `微信图片_xxx.png`, `screenshot.jpg`), first resolve the path (may be relative to workspace root or absolute).
2. Run the analysis script:
   ```bash
   python scripts/analyze_image.py "<full_path_to_image>"
   ```
3. Present the results to the user in a readable format.

## What the script returns
- **File metadata**: format, dimensions, color mode, file size
- **Image analysis**: average brightness, edge density, dominant colors (via OpenCV)
- **OCR text**: Chinese + English text recognition (via EasyOCR GPU) — great for screenshots, dialog boxes, error messages

## First-time setup (one time only)
The EasyOCR models need to be downloaded once. If the user has VPN/proxy on:
```bash
python scripts/analyze_image.py --download-models
```

## Edge cases
- If the file doesn't exist, tell the user the path is wrong
- If it's not a valid image, the script will return an error
- For very large images, OCR may take a few seconds (GPU accelerated)
- The script supports: PNG, JPG, JPEG, BMP, GIF, WebP, TIFF
- If OCR models aren't cached, the script will report "OCR: models not cached" and tell the user how to download them
