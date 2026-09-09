from pathlib import Path
import sys
from PIL import Image
from PyQt5.QtWidgets import QApplication
from PyQt5.QtSvg import QSvgRenderer
from PyQt5.QtGui import QImage, QPainter
from PyQt5.QtCore import Qt, QRectF, QByteArray

ROOT = Path(__file__).resolve().parent
ICONS = ROOT / "icons"
SOURCE_SVG = ICONS / "Flow.svg"
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]
app = QApplication(sys.argv)

def render_svg(svg_path, size, pad_ratio=0.08):
    """Render SVG into a square RGBA image with optional transparent padding."""
    ren = QSvgRenderer(QByteArray(svg_path.read_bytes()))
    img = QImage(size, size, QImage.Format_ARGB32)
    img.fill(Qt.transparent)
    p = QPainter(img)
    p.setRenderHint(QPainter.Antialiasing, True)
    margin = int(round(size * pad_ratio))
    ren.render(p, QRectF(margin, margin, size - 2 * margin, size - 2 * margin))
    p.end()
    ptr = img.bits()
    ptr.setsize(img.byteCount() if hasattr(img, "byteCount") else img.sizeInBytes())
    return Image.frombuffer("RGBA", (size, size), bytes(ptr), "raw", "BGRA", 0, 1).copy()

render_svg(SOURCE_SVG, 512, pad_ratio=0.06).save(ICONS / "Flow.png")
render_svg(SOURCE_SVG, 256, pad_ratio=0.06).save(ROOT / "res.ico", format="ICO", sizes=[(s, s) for s in ICO_SIZES])
print("done:", SOURCE_SVG.name, "-> Flow.png, res.ico")
