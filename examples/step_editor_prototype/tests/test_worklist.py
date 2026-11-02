from pathlib import Path

from app.worklist import next_unconditioned, raw_models


def _touch(folder: Path, *names: str) -> None:
    for name in names:
        (folder / name).write_text("x", encoding="utf-8")


def test_follows_worklist_order_and_skips_done(tmp_path: Path) -> None:
    _touch(tmp_path, "a.STEP", "b.STEP", "c.STEP", "d.step", "b_AP242_WNC1.step", "notes.txt")
    (tmp_path / "worklist.csv").write_text("rank,copy\n1,c.STEP\n2,a.STEP\n3,b.STEP\n", encoding="utf-8")
    assert [p.name for p in raw_models(tmp_path)] == ["c.STEP", "a.STEP", "b.STEP", "d.step"]
    nxt, left = next_unconditioned(tmp_path / "c.STEP")
    assert nxt.name == "a.STEP" and left == 2          # b is done; a and d remain
    nxt, left = next_unconditioned(tmp_path / "a.STEP")
    assert nxt.name == "d.step" and left == 2          # wraps past done b to d, then c


def test_from_an_export_and_when_finished(tmp_path: Path) -> None:
    _touch(tmp_path, "a.STEP", "a_AP242_WNC2.step", "b.STEP", "b_AP242_WNC1.step")
    assert next_unconditioned(tmp_path / "a_AP242_WNC2.step") == (None, 0)
    _touch(tmp_path, "c.STEP")
    nxt, left = next_unconditioned(tmp_path / "a_AP242_WNC2.step")
    assert nxt.name == "c.STEP" and left == 1
