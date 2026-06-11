"""WN3D STEP Conditioning Editor (prototype).

Run the GUI:
    uv run --project examples/step_editor_prototype python examples/step_editor_prototype/step_editor.py <file.step>

Headless milestone selftests:
    ... step_editor.py --selftest m0 [--fixture <file.step>]

Replay a recorded operation journal headlessly (lands in M8):
    ... step_editor.py --apply ops.json <file.step>
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    parser = argparse.ArgumentParser(description="WN3D STEP Conditioning Editor prototype")
    parser.add_argument("step", nargs="?", default=None, help="STEP/STP file to open")
    parser.add_argument("--selftest", metavar="NAME", help="run a headless selftest (m0..m8 or 'all')")
    parser.add_argument("--fixture", type=Path, default=None, help="override the selftest fixture")
    parser.add_argument("--apply", type=Path, default=None, metavar="OPS_JSON",
                        help="replay an operation journal headlessly (M8)")
    args = parser.parse_args()

    if args.selftest:
        from app.selftest import run

        return run(args.selftest, args.fixture)

    if args.apply:
        if not args.step:
            print("--apply needs an input STEP file")
            return 2
        from app.replay import apply_journal_file

        report, registry = apply_journal_file(Path(args.step), args.apply)
        print(f"replayed journal: {len(registry.pins)} pins | {report.summary()}")
        return 0 if report.ok else 1

    from PySide6.QtGui import QIcon
    from PySide6.QtWidgets import QApplication

    from app.app_window import LOGO_PATH, MainWindow
    from app.style import apply_wavenumber_style

    step_path = Path(args.step) if args.step else None
    app = QApplication.instance() or QApplication(sys.argv[:1])
    try:  # taskbar identity: our logo instead of the generic Python icon
        import ctypes

        ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(
            "Wavenumber.StepEditor")
    except Exception:
        pass
    if LOGO_PATH.is_file():
        app.setWindowIcon(QIcon(str(LOGO_PATH)))
    apply_wavenumber_style(app)
    window = MainWindow(step_path if step_path and step_path.is_file() else None)
    window.show()
    return int(app.exec())


if __name__ == "__main__":
    raise SystemExit(main())
