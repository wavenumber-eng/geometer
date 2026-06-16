"""The Additive Design Intent identity laws, distilled:

    [1] container -> [2] AP242 -> [3] container -> [4] AP242 -> [5] container
    with [2]==[4] and [3]==[5] line for line, and [4] carrying metadata.

These run on synthetic payloads so they are fast and fixture-free — the bake
layer is payload-agnostic, which is itself the property that makes the laws
achievable (conditioned AP242 is opaque bytes; no re-export, no
normalization). The full chain with REAL conditioning runs in the kicad_rt /
altium_rt selftests (see test_past_milestones)."""

from __future__ import annotations

from app.export_ap242 import conditioned_path
from app.kicad_embed import (
    bake_step_into_kicad_mod,
    decode_payload,
    find_embedded_files,
    scaffold_kicad_mod,
)
from conftest import make_synthetic_payload


class TestChainIdentities:
    def test_full_chain_identities(self, tmp_path):
        raw = make_synthetic_payload(b"RAW", 30_000)
        conditioned = make_synthetic_payload(b"CONDITIONED", 50_000)

        # [1] the container as received
        mod1 = tmp_path / "part.kicad_mod"
        with open(mod1, "w", encoding="utf-8", newline="") as fh:
            fh.write(scaffold_kicad_mod("part", "part.step", raw))

        # extract from [1] must be byte-identical to what was embedded
        with open(mod1, encoding="utf-8", newline="") as fh:
            step1 = decode_payload(find_embedded_files(fh.read())[0].data_base64)
        assert step1 == raw

        # [2] -> bake -> [3]
        mod3 = tmp_path / "part_3.kicad_mod"
        bake_step_into_kicad_mod(str(mod1), conditioned, str(mod3))

        # [3] -> extract -> [4]: must equal [2] line for line
        with open(mod3, encoding="utf-8", newline="") as fh:
            step4 = decode_payload(find_embedded_files(fh.read())[0].data_base64)
        assert step4 == conditioned  # [2] == [4]

        # [4] -> bake -> [5]: must equal [3] line for line
        mod5 = tmp_path / "part_5.kicad_mod"
        bake_step_into_kicad_mod(str(mod3), step4, str(mod5))
        assert mod5.read_bytes() == mod3.read_bytes()  # [3] == [5]

    def test_bake_is_deterministic_across_runs(self, tmp_path):
        payload = make_synthetic_payload(b"DET", 10_000)
        mod = tmp_path / "p.kicad_mod"
        with open(mod, "w", encoding="utf-8", newline="") as fh:
            fh.write(scaffold_kicad_mod("p", "p.step", make_synthetic_payload(b"X")))
        out_a, out_b = tmp_path / "a.kicad_mod", tmp_path / "b.kicad_mod"
        bake_step_into_kicad_mod(str(mod), payload, str(out_a))
        bake_step_into_kicad_mod(str(mod), payload, str(out_b))
        assert out_a.read_bytes() == out_b.read_bytes()

    def test_conditioned_bytes_are_opaque(self, tmp_path):
        # The bake must not normalize line endings, whitespace, or encoding
        # of the AP242 payload — mixed endings and odd bytes survive exactly.
        payload = b"ISO-10303-21;\r\nDATA;\n\tweird \x80\xff bytes\rENDSEC;"
        mod = tmp_path / "p.kicad_mod"
        with open(mod, "w", encoding="utf-8", newline="") as fh:
            fh.write(scaffold_kicad_mod("p", "p.step", b"old"))
        out = tmp_path / "o.kicad_mod"
        bake_step_into_kicad_mod(str(mod), payload, str(out))
        with open(out, encoding="utf-8", newline="") as fh:
            assert decode_payload(find_embedded_files(fh.read())[0].data_base64) == payload


class TestNamingLaws:
    def test_ap242_output_naming(self, tmp_path):
        # DESIGN_INTENT: a raw source conditions to <base>_AP242_WNC1.step,
        # next to the one loaded in.
        src = tmp_path / "wheel4506.step"
        out = conditioned_path(src)
        assert out.name == "wheel4506_AP242_WNC1.step"
        assert out.parent == src.parent

    def test_reconditioning_collapses_and_increments(self, tmp_path):
        # Re-conditioning never stacks suffixes — the tag collapses and the
        # version bumps. Legacy `_AP242_conditioned` counts as version 1.
        assert conditioned_path(tmp_path / "X_AP242_WNC1.step").name == "X_AP242_WNC2.step"
        assert conditioned_path(tmp_path / "X_AP242_WNC3.step").name == "X_AP242_WNC4.step"
        assert conditioned_path(tmp_path / "X_AP242_conditioned.step").name == "X_AP242_WNC2.step"
