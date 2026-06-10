"""Standard Wavenumber styling, ported from the wavenumber-browse-3d app:
near-black panels, amber accent, muted-blue selection, Berkeley Mono.

Berkeley Mono is a licensed font, so the TTFs are NOT committed to this repo:
they load from a local (git-ignored) assets/fonts/ folder or from the
wavenumber-browse-3d skill install, falling back to Consolas.
"""

from __future__ import annotations

from pathlib import Path

from PySide6.QtGui import QColor, QFontDatabase, QPalette

# Palette — identical to wn3d_browser.py
WINDOW_BG = "#0a0a0a"
PANEL_BG = "#0d0d0d"
CONSOLE_BG = "#0d0d0d"
BORDER = "#282832"
TEXT_PRIMARY = "#e7dddd"
TEXT_MUTED = "#969696"
ACCENT = "#b48700"          # amber/gold
ACCENT_HOVER = "#c89d00"
SELECT_BG = "#4a5aa0"       # muted blue selection
ROW_HOVER = "#1a1a1f"
SUCCESS = "#64ff64"
ERROR = "#ff6464"
VIEWPORT_BG = (0.04, 0.04, 0.05)

HERE = Path(__file__).resolve().parent.parent
FONT_DIRS = [
    HERE / "assets" / "fonts",
    Path.home() / ".claude" / "skills" / "wavenumber-browse-3d" / "assets" / "fonts",
]
FONT_FILES = ("BerkeleyMono-Regular.ttf", "BerkeleyMono-Bold.ttf")


def _load_fonts() -> str:
    for directory in FONT_DIRS:
        loaded = None
        for name in FONT_FILES:
            path = directory / name
            if path.is_file():
                font_id = QFontDatabase.addApplicationFont(str(path))
                families = QFontDatabase.applicationFontFamilies(font_id)
                if families:
                    loaded = families[0]
        if loaded:
            return loaded
    return "Consolas"


def stylesheet(family: str) -> str:
    return f"""
QWidget {{
    background-color: {PANEL_BG};
    color: {TEXT_PRIMARY};
    font-family: "{family}";
    font-size: 10pt;
}}
QMainWindow, QStatusBar {{ background-color: {WINDOW_BG}; }}
QStatusBar {{ color: {TEXT_MUTED}; border-top: 1px solid {BORDER}; }}

QLabel {{ background: transparent; }}

QPushButton {{
    background-color: {PANEL_BG};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    padding: 4px 10px;
}}
QPushButton:hover {{ background-color: {ACCENT_HOVER}; color: #000000; }}
QPushButton:pressed {{ background-color: {ACCENT}; color: #000000; }}
QPushButton:disabled {{ color: {TEXT_MUTED}; border-color: {BORDER}; }}

QComboBox {{
    background-color: {PANEL_BG};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    padding: 2px 6px;
}}
QComboBox:hover {{ border-color: {ACCENT}; }}
QComboBox QAbstractItemView {{
    background-color: {PANEL_BG};
    color: {TEXT_PRIMARY};
    selection-background-color: {SELECT_BG};
    selection-color: #ffffff;
    border: 1px solid {BORDER};
}}

QDoubleSpinBox, QSpinBox, QLineEdit {{
    background-color: {CONSOLE_BG};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
    padding: 2px 4px;
    selection-background-color: {SELECT_BG};
}}

QListWidget {{
    background-color: {CONSOLE_BG};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
}}
QListWidget::item {{ padding: 2px 4px; }}
QListWidget::item:hover {{ background-color: {ROW_HOVER}; }}
QListWidget::item:selected {{ background-color: {SELECT_BG}; color: #ffffff; }}

QCheckBox {{ background: transparent; spacing: 6px; }}
QCheckBox::indicator {{
    width: 12px; height: 12px;
    border: 1px solid {BORDER};
    background-color: {CONSOLE_BG};
}}
QCheckBox::indicator:checked {{ background-color: {ACCENT}; }}

QMenu {{
    background-color: {PANEL_BG};
    color: {TEXT_PRIMARY};
    border: 1px solid {BORDER};
}}
QMenu::item:selected {{ background-color: {SELECT_BG}; color: #ffffff; }}

QSplitter::handle {{ background-color: {BORDER}; }}
QSplitter::handle:horizontal {{ width: 2px; }}
QSplitter::handle:vertical {{ height: 2px; }}

QToolTip {{
    background-color: {PANEL_BG};
    color: {TEXT_PRIMARY};
    border: 1px solid {ACCENT};
}}

QScrollBar:vertical {{
    background: {PANEL_BG}; width: 10px; margin: 0;
}}
QScrollBar::handle:vertical {{
    background: {BORDER}; min-height: 24px;
}}
QScrollBar::handle:vertical:hover {{ background: {ACCENT}; }}
QScrollBar:horizontal {{
    background: {PANEL_BG}; height: 10px; margin: 0;
}}
QScrollBar::handle:horizontal {{ background: {BORDER}; min-width: 24px; }}
QScrollBar::add-line, QScrollBar::sub-line {{ width: 0; height: 0; }}
"""


def apply_wavenumber_style(app) -> str:
    """Register fonts + apply the stylesheet. Returns the font family used."""
    family = _load_fonts()
    app.setStyleSheet(stylesheet(family))
    palette = app.palette()
    palette.setColor(QPalette.ColorRole.Window, QColor(WINDOW_BG))
    palette.setColor(QPalette.ColorRole.WindowText, QColor(TEXT_PRIMARY))
    palette.setColor(QPalette.ColorRole.Base, QColor(CONSOLE_BG))
    palette.setColor(QPalette.ColorRole.Text, QColor(TEXT_PRIMARY))
    palette.setColor(QPalette.ColorRole.Highlight, QColor(SELECT_BG))
    palette.setColor(QPalette.ColorRole.HighlightedText, QColor("#ffffff"))
    app.setPalette(palette)
    return family
