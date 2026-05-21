"""Analyze an image file: metadata, color analysis, edge detection, OCR."""
import sys, os, json, subprocess
from pathlib import Path

from PIL import Image
import cv2
import numpy as np

EASYOCR_DIR = Path.home() / ".EasyOCR" / "model"
GH_MIRROR = os.environ.get("GH_MIRROR", "https://ghproxy.com/")
MODEL_URLS = {
    "craft_mlt_25k.pth": GH_MIRROR + "https://github.com/JaidedAI/EasyOCR/releases/download/pre-v1.1.6/craft_mlt_25k.zip",
    "english_g2.pth": GH_MIRROR + "https://github.com/JaidedAI/EasyOCR/releases/download/v1.3/english_g2.zip",
    "zh_sim_g2.pth": GH_MIRROR + "https://github.com/JaidedAI/EasyOCR/releases/download/v1.3/zh_sim_g2.zip",
}


def _models_cached() -> bool:
    if not EASYOCR_DIR.is_dir():
        return False
    files = [f for f in EASYOCR_DIR.iterdir() if f.suffix == ".pth"]
    return len(files) >= 3


def _setup_easyocr():
    if not _models_cached():
        return None
    try:
        import easyocr
        return easyocr.Reader(['ch_sim', 'en'], gpu=True)
    except Exception:
        return None


_reader = _setup_easyocr()


def download_models():
    """Download EasyOCR models with proxy support."""
    import requests, zipfile, io
    EASYOCR_DIR.mkdir(parents=True, exist_ok=True)
    proxies = {
        "http": os.environ.get("HTTP_PROXY") or os.environ.get("http_proxy") or "",
        "https": os.environ.get("HTTPS_PROXY") or os.environ.get("https_proxy") or "",
    }
    ok = True
    for fname, url in MODEL_URLS.items():
        dest = EASYOCR_DIR / fname
        if dest.exists() and dest.stat().st_size > 100000:
            print(f"[OK] {fname} already cached")
            continue
        print(f"Downloading {fname} ...")
        try:
            r = requests.get(url, proxies=proxies if any(proxies.values()) else None, timeout=120, stream=True)
            r.raise_for_status()
            z = zipfile.ZipFile(io.BytesIO(r.content))
            z.extract(fname, EASYOCR_DIR)
            z.close()
            print(f"  -> saved {dest.stat().st_size} bytes")
        except Exception as e:
            print(f"  FAILED: {e}")
            ok = False
    if ok:
        print("All models downloaded. Restart opencode and try again.")
    else:
        print("Some models failed. Connect VPN and run:")
        print("  python scripts/analyze_image.py --download-models")


def analyze(path: str) -> dict:
    p = Path(path)
    if not p.exists():
        return {"error": f"File not found: {path}"}

    result = {"file": str(p.resolve()), "size_bytes": p.stat().st_size}

    img = Image.open(p)
    result["format"] = img.format or "unknown"
    result["mode"] = img.mode
    result["width"] = img.width
    result["height"] = img.height

    cv_img = cv2.imread(str(p))
    if cv_img is not None:
        h, w, c = cv_img.shape
        result["channels"] = c

        gray = cv2.cvtColor(cv_img, cv2.COLOR_BGR2GRAY)
        result["mean_brightness"] = round(float(gray.mean()), 1)

        edges = cv2.Canny(gray, 50, 150)
        edge_ratio = float((edges > 0).sum()) / (h * w)
        result["edge_density"] = round(edge_ratio, 4)

        lab = cv2.cvtColor(cv_img, cv2.COLOR_BGR2LAB)
        l, a, b = cv2.split(lab)
        result["lab_mean"] = {
            "L": round(float(l.mean()), 1),
            "A": round(float(a.mean()), 1),
            "B": round(float(b.mean()), 1),
        }

        pixels = cv_img.reshape(-1, 3)
        k = min(5, len(pixels))
        if k >= 5:
            criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 10, 1.0)
            _, labels, centers = cv2.kmeans(
                pixels.astype(np.float32), k, None, criteria, 10,
                cv2.KMEANS_RANDOM_CENTERS
            )
            counts = np.bincount(labels.flatten())
            result["dominant_colors"] = [
                {"bgr": [int(v) for v in c], "ratio": round(float(counts[i]) / len(labels), 3)}
                for i, c in enumerate(centers)
            ]
            result["dominant_colors"].sort(key=lambda x: -x["ratio"])

    if _reader is not None:
        try:
            ocr_result = _reader.readtext(path)
            texts = []
            for bbox, text, confidence in ocr_result:
                if confidence >= 0.3:
                    texts.append({"text": text, "confidence": round(confidence, 3)})
            result["ocr_texts"] = texts
        except Exception as e:
            result["ocr_error"] = str(e)
    else:
        result["ocr_texts"] = []
        result["ocr_models_missing"] = True

    return result


def format_result(result: dict) -> str:
    if "error" in result:
        return f"[ERROR] {result['error']}"

    lines = []
    lines.append(f"File: {result['file']}")
    lines.append(f"Size: {result['size_bytes']} bytes")
    lines.append(f"Format: {result['format']} | Mode: {result['mode']}")
    lines.append(f"Dimensions: {result['width']} x {result['height']} px")

    if "channels" in result:
        lines.append(f"Channels: {result['channels']}")
        lines.append(f"Brightness: {result['mean_brightness']}")
        lines.append(f"Edge density: {result['edge_density']}")
        L, A, B = result['lab_mean']['L'], result['lab_mean']['A'], result['lab_mean']['B']
        lines.append(f"LAB: L={L} A={A} B={B}")

    if "dominant_colors" in result:
        lines.append("Dominant colors (BGR):")
        for c in result["dominant_colors"][:3]:
            lines.append(f"  ({c['bgr'][0]},{c['bgr'][1]},{c['bgr'][2]}) = {c['ratio']*100:.1f}%")

    if result.get("ocr_models_missing"):
        lines.append("OCR: models not cached. Connect VPN and run:")
        lines.append(f"  python {__file__} --download-models")
    elif result.get("ocr_texts"):
        lines.append("OCR text:")
        for t in result["ocr_texts"]:
            lines.append(f"  [{t['confidence']:.0%}] {t['text']}")
    else:
        lines.append("OCR: no text found")

    return "\n".join(lines)


if __name__ == "__main__":
    if "--download-models" in sys.argv:
        download_models()
        sys.exit(0)
    if len(sys.argv) < 2:
        print(f"Usage: python {__file__} <image_path>")
        print(f"       python {__file__} --download-models")
        sys.exit(1)
    path = sys.argv[1]
    print(format_result(analyze(path)))
