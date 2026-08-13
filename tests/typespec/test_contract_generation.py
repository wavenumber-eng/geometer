from __future__ import annotations

import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def test_generated_contracts_are_current() -> None:
    npm = "npm.cmd" if os.name == "nt" else "npm"
    subprocess.run(
        [npm, "run", "check:contracts"],
        cwd=ROOT,
        check=True,
    )
