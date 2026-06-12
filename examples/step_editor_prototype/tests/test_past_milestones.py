"""The original design intent stays valid: every selftest in the registry —
M0 shell/picking/export, Z-sit, pin-1 quadrant, detect pins, hitboxes, pin
functions, colors, Separate, LOGO, metadata+replay (M8), the headless auto
conditioner, and the [1]→[5] container chains — runs as a pytest case, so
any future change that breaks a past milestone fails this suite.

These are integration-weight (the full run is a few minutes). Deselect them
for a quick pure-unit pass with:  pytest -q -k "not past_milestone"
"""

from __future__ import annotations

import pytest

from app.selftest import SELFTESTS, run


@pytest.mark.parametrize("name", list(SELFTESTS))
def test_past_milestone(name):
    assert run(name) == 0, f"selftest {name} regressed"
