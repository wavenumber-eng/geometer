"""Offline package / lead-count parser.

Read a STEP filename and, when it encodes a standard package (SOIC-8, SOT23-5,
TQFP...-100L, VFBGA176, 153BallWFBGA, ...), return the {archetype, leads} it
implies. This is a PRIOR for the pin detector — a target pin count to meet and
an archetype to route its decision tree — never ground truth: the geometry
rules, and the hint only sets a goal, flags count mismatches, and disambiguates
look-alikes. Pure and offline (no network); returns None when the name yields
nothing reliable so the detector falls back to pure geometry.

Once the detector knows the target N, a set (or a few equal sets) of similar-
area faces numbering N confirms those faces ARE the pins — the count closes the
loop the geometry alone leaves open.
"""

from __future__ import annotations

import re

# Archetype keyword -> canonical archetype, tried IN ORDER: passives and diodes
# win over the generic SO* family so SOD-123 reads as a 2-lead diode, not SOIC.
_ARCHETYPE = [
    ("passive", re.compile(r"^[CRL]\d{3,4}|RESC|CAPC|C_TANT", re.I)),
    ("diode", re.compile(r"\bSOD\b|SOD-?\d|\bDO-?\d{3}|POWERDI|\bSM[ABC]\b", re.I)),
    ("bga", re.compile(r"BGA|\d+\s*BALL", re.I)),
    ("qfp", re.compile(r"QFP", re.I)),
    ("qfn", re.compile(r"[QD]FN", re.I)),
    ("soic", re.compile(r"SOIC|\bSOP\b|SSOP|MSOP", re.I)),
    ("sot", re.compile(r"\bSOT\b|SOT-?23|SOT\d", re.I)),
    ("crystal", re.compile(r"CRYSTAL|\bOSC\b|ABM\d|ECS-|ECX|\dMHZ", re.I)),
    ("tact", re.compile(r"TACT|BUTTON|SWITCH", re.I)),
]

# Archetypes whose lead count is fixed by the part class (the digits in their
# names are EIA/package codes, not pin counts).
_FIXED = {"passive": 2, "diode": 2}

# IPC-7351 form: <pitch>P<body>X<body>...-<COUNT><density>. The count is the
# trailing number before an optional density letter (L/M/N/H), NOT the leading
# pitch — TSSOP50P810X120-56M is 56 leads at 0.50 mm pitch.
_IPC = re.compile(r"\d+P\d+X.*?-(\d+)[A-Z]*$", re.I)
# BGA: ball count may sit before or after the BGA token.
# (the before-BGA count must be >=2 digits so a "-1" variant suffix in
# "SOT1850-1_VFBGA176" isn't mistaken for a 1-ball part — ball counts are never
# single-digit, while the "176" after BGA is the real count.)
_BALL = re.compile(r"(\d+)\s*BALL|BGA[-_ ]?(\d+)|(\d{2,})[-_ ]?[VWF]*BGA", re.I)
# Named-package count, keyed by archetype: the number MUST sit against that
# package's token, so "SOT905-1_HVQFN24" reads 24 (the QFN), not 905 (the SOT
# outline code), and a bare "SOT-23" yields no count (it's the package, not 23).
_NAMED_BY_ARCH = {
    "soic": re.compile(r"(?:SOIC|SOP|SSOP|TSSOP|VSSOP|MSOP)-?(\d+)", re.I),
    "qfp": re.compile(r"QFP-?(\d+)", re.I),
    "qfn": re.compile(r"[QD]FN-?(\d+)", re.I),
    "sot": re.compile(r"SOT-?23-(\d+)", re.I),
}


def _archetype(stem: str) -> str | None:
    for archetype, pattern in _ARCHETYPE:
        if pattern.search(stem):
            return archetype
    return None


def _first_group(match) -> int:
    return int(next(group for group in match.groups() if group))


def _lead_count(stem: str, archetype: str | None) -> int | None:
    if archetype in _FIXED:
        return _FIXED[archetype]
    if archetype == "bga":
        match = _BALL.search(stem)
        return _first_group(match) if match else None
    ipc = _IPC.search(stem)
    if ipc:
        return int(ipc.group(1))
    named = _NAMED_BY_ARCH.get(archetype or "")
    match = named.search(stem) if named else None
    return int(match.group(1)) if match else None


def package_hint(name: str) -> dict | None:
    """{'archetype': str|None, 'leads': int|None} parsed from a STEP filename,
    or None when the name yields nothing reliable. A detection PRIOR, not truth.
    A plausibility clamp keeps a stray number from posing as a 5000-pin part."""
    stem = name.rsplit(".", 1)[0]
    archetype = _archetype(stem)
    leads = _lead_count(stem, archetype)
    if leads is not None and not 2 <= leads <= 2048:
        leads = None
    if archetype is None and leads is None:
        return None
    return {"archetype": archetype, "leads": leads}
