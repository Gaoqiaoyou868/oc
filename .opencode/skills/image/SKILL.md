---
name: image
description: "Analyze and describe image files using the Xiaomi MIMO vision model (mimo-v2.5) via MCP. Use when the user provides an image file path, screenshot, or asks 'what is in this image', '帮我看看这张图', '读取图片', or references an image file (.png, .jpg, .jpeg, .bmp, .gif, .webp)."
---

# Image Analysis Skill (MIMO Vision)

## When to use
Use this skill when the user gives you an image file path, pastes a screenshot, or asks you to look at an image. The model itself cannot see images, so use the MIMO vision MCP tool (`mimo-vision_recognize_image`) to analyze the image.

## Workflow
1. When user references an image file (e.g. `微信图片_xxx.png`, `screenshot.jpg`), first resolve the path (may be relative to workspace root or absolute).
2. Call the MIMO vision tool:
   ```
   mimo-vision_recognize_image with image_path="<full_path_to_image>"
   ```
   Optionally, set `question` to a specific query about the image content.
3. Present the results to the user in a readable format.

## Edge cases
- If the file doesn't exist, tell the user the path is wrong
- If it's not a valid image, the tool will return an error
- The tool supports: PNG, JPG, JPEG, BMP, GIF, WebP, TIFF
- For large images, processing may take a few seconds (cloud-based MIMO v2.5 model)
