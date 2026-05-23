from __future__ import annotations

import sys
from pathlib import Path

from setuptools import setup
from setuptools.command.build_py import build_py as _build_py
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel


ROOT = Path(__file__).resolve().parent


class build_py(_build_py):
    def run(self) -> None:
        super().run()
        executable = _source_executable()
        target_dir = Path(self.build_lib) / "geometer" / "native"
        target_dir.mkdir(parents=True, exist_ok=True)
        self.copy_file(str(executable), str(target_dir / executable.name))


class bdist_wheel(_bdist_wheel):
    def finalize_options(self) -> None:
        super().finalize_options()
        self.root_is_pure = False


def _source_executable() -> Path:
    name = "geometer.exe" if sys.platform == "win32" else "geometer"
    path = ROOT / "dist" / name
    if not path.exists():
        raise FileNotFoundError(
            f"Missing {path}. Build the native CLI before building the Python wheel."
        )
    return path


setup(cmdclass={"build_py": build_py, "bdist_wheel": bdist_wheel})
