"""MagJack-style jacks: the contacts in the opening are numbered J1..Jn along
their row (J1 at +X by default, flip to start at the other end) and never
joined to a PCB tail."""

from app.pins import Pin, contact_row_order


def _row(xs, y=7.0):
    return [Pin(number=0, centroid=(x, y, 9.2), role="mouth") for x in xs]


def test_j1_at_plus_x_and_flip_reverses():
    contacts = _row([-1.5, 3.5, 0.5, -3.5, 2.5, -0.5, 1.5, -2.5])
    order = contact_row_order(contacts)
    assert [contacts[i].centroid[0] for i in order] == [3.5, 2.5, 1.5, 0.5, -0.5, -1.5, -2.5, -3.5]
    assert contact_row_order(contacts, flip=True) == order[::-1]


def test_row_along_y_starts_at_plus_y():
    contacts = [Pin(number=0, centroid=(0.0, y, 1.0), role="mouth") for y in (-1.0, 2.0, 0.5)]
    order = contact_row_order(contacts)
    assert [contacts[i].centroid[1] for i in order] == [2.0, 0.5, -1.0]
