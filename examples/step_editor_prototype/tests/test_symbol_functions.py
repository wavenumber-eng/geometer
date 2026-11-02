"""Pin types from a schematic symbol pin: by name first, then by the
symbol's electrical type; passive pins with no telling name stay blank."""

import pytest

from app.tools.pin_functions import function_from_symbol


@pytest.mark.parametrize("name, kind, expected", [
    ("GND", "Passive", "GND"), ("AGND", "Power", "AGND"), ("VDD", "Passive", "VDD"),
    ("SDA/OUT1", "Passive", "SDA"), ("SCL/OUT2", "I/O", "SCL"), ("VBUS_DET", "Passive", "VBUS"),
    ("EN", "Input", "IO"), ("VSYS", "Power", "PWR"), ("CC1", "Passive", ""),
])
def test_function_from_symbol(name, kind, expected):
    assert function_from_symbol(name, kind) == expected
