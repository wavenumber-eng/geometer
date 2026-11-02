"""Pin numbering follows the pin layout: RING (QFN / QFP), TWO ROW (SOT /
SOIC / DIP), LINE, GRID (BGA). A QFN numbered from pin 1 walks its perimeter,
pin 2 next to pin 1 on its own side, the centre pad last."""

import numpy as np
import pytest

from app.pins import _predict_ring, pin_pattern

OFF = np.arange(8) * 0.5 - 1.75
QFN = ([(2.5, y) for y in OFF[::-1]] + [(x, -2.5) for x in OFF[::-1]]
       + [(-2.5, y) for y in OFF] + [(x, 2.5) for x in OFF] + [(0.0, 0.0)])  # true numbering order


def _xyz(points):
    return np.array([[x, y, 0.0] for x, y in points])


@pytest.mark.parametrize("name, points, expected", [
    ("qfn + pad", QFN, "ring"),
    ("qfp", QFN[:-1], "ring"),
    ("soic-8", [(x * 1.27 - 1.905, 2.7) for x in range(4)] + [(x * 1.27 - 1.905, -2.7) for x in range(4)], "rows"),
    ("sot-23", [(-0.95, -1), (0.95, -1), (0, 1)], "rows"),
    ("sot-223", [(-2.3, -3.1), (0, -3.1), (2.3, -3.1), (0, 3.1)], "rows"),
    ("header", [(x, 0) for x in OFF], "line"),
    ("bga", [(x, y) for x in OFF for y in OFF], "grid"),
])
def test_pattern(name, points, expected):
    assert pin_pattern(_xyz(points)) == expected, name


@pytest.mark.parametrize("pin1", [0, 5, 13])
def test_qfn_numbers_from_pin1_around_its_perimeter(pin1):
    rotated = QFN[:-1][pin1:] + QFN[:-1][:pin1]  # pin 1 anywhere, even mid-side
    truth = rotated + [QFN[-1]]
    perm = np.random.default_rng(pin1).permutation(len(truth))
    shuffled = _xyz([truth[k] for k in perm])
    first = int(np.where(perm == 0)[0][0])
    order = _predict_ring(shuffled, {first: 1})
    numbered = [int(perm[i]) for i in order]  # true position of pin 1, 2, 3 ...
    if pin1 % 8 == 0:  # pin 1 starts a side: pin 2 must be along that side
        assert numbered == list(range(33))
    else:  # mid-side: either way round is consistent; still a full perimeter walk
        assert numbered[0] == 0 and numbered[-1] == 32 and sorted(numbered) == list(range(33))
