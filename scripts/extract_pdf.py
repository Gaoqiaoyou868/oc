"""用 pymupdf 提取 STC89Cxx 中文参考手册的全部文本"""
import fitz
import os

pdf_path = r"C:\Users\17537\Desktop\cc\STC89Cxx中文参考手册.pdf"
out_path = r"C:\Users\17537\Desktop\cc\stc_manual_clean.txt"

doc = fitz.open(pdf_path)
total = len(doc)
print(f"总页数: {total}")

with open(out_path, "w", encoding="utf-8") as f:
    for page_num in range(total):
        page = doc[page_num]
        text = page.get_text("text")
        f.write(f"\n{'='*60}\n")
        f.write(f"=== 第 {page_num+1} 页 / 共 {total} 页 ===\n")
        f.write(f"{'='*60}\n\n")
        f.write(text)

doc.close()
print(f"提取完成 → {out_path}")
print(f"文件大小: {os.path.getsize(out_path)} 字节")
