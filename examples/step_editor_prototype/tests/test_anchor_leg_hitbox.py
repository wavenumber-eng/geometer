"""A face-marked ANCHOR hangs from a bigger body (a shield): its hitbox follows
the metal from the tip up to the Z-Sit plane, and only its own leg."""

import numpy as np
import pyvista as pv

from app.hitbox import _connected_below


class _Mesh:
    def __init__(self, poly):
        tri = poly.triangulate()
        self.points = np.asarray(tri.points)
        self.tris = tri.faces.reshape(-1, 4)[:, 1:]


def _shield():
    """A plate above the board (z 0..1) on two legs down to z = -3."""
    plate = pv.Box(bounds=(-5, 5, -1, 1, 0, 1))
    left = pv.Box(bounds=(-4.2, -3.8, -0.2, 0.2, -3, 0))
    right = pv.Box(bounds=(3.8, 4.2, -0.2, 0.2, -3, 0))
    return _Mesh(plate.merge([left, right]).clean())


def test_leg_runs_from_the_tip_up_to_the_seat_and_stays_on_its_own_leg():
    points = _connected_below(_shield(), np.array([-4.0, 0.0, -3.0]))
    assert points is not None
    lo, hi = points.min(axis=0), points.max(axis=0)
    assert lo[2] <= -2.99 and abs(hi[2]) < 1e-6     # tip to Z = 0, nothing above
    assert lo[0] >= -4.21 and hi[0] <= -3.79        # the left leg only


def _lead(points):
    import numpy as np
    return np.asarray(points, dtype=float)


def test_rule_shapes_follow_the_lead():
    """Gull-wing / DIP legs (long, hull >> metal) get a shrink wrap; short L
    QFN terminals (length / thickness 2.0) stay hulls; balls get boxes."""
    import types

    import numpy as np

    from app import hitbox

    class _Doc:
        path = "SOIC-8.STEP"

    gull = _lead([[0, 0, 0], [1.0, 0, 0], [1.0, 0.25, 0], [0, 0.25, 0], [0, 0, 0.6], [0.2, 0.25, 0.6]])
    assert float(np.ptp(gull, axis=0).max()) >= hitbox.BENT_ELONGATION * float(np.ptp(gull, axis=0).min())
    qfn = _lead([[0, 0, 0], [0.4, 0, 0], [0.4, 0.2, 0], [0, 0.2, 0.2]])
    assert float(np.ptp(qfn, axis=0).max()) < hitbox.BENT_ELONGATION * float(np.ptp(qfn, axis=0).min())
    ball = _lead([[0, 0, 0], [0.3, 0, 0], [0, 0.3, 0], [0, 0, 0.28], [0.3, 0.3, 0.28]])
    doc = types.SimpleNamespace(path="Texas_DSBGA-10.STEP")
    assert hitbox.is_ball(doc, types.SimpleNamespace(kind="pin"), ball)
    assert not hitbox.is_ball(_Doc(), types.SimpleNamespace(kind="pin"), ball)
