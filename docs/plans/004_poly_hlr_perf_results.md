# Poly HLR perf — results

- View: `top`
- Models: 36
- Generated: 2026-05-16T15:05:55.296Z

## TL;DR

Switching the default HLR algorithm from `HLRBRep_Algo` (exact) to
`HLRBRep_PolyAlgo` (mesh-driven, 0.01 mm linear deflection) yields a **4.02x
overall corpus speedup** with no schema or contract changes. The biggest
wins land on dense, BGA-style models where the exact algorithm previously
spent tens of seconds:

| Model | Baseline | 1.1 | Speedup |
|---|---:|---:|---:|
| JDEC_153BallWFBGA_eMMC.step | 47.55 s | 2.31 s | **20.6x** |
| BGA90-8X13mm.step | 23.96 s | 1.25 s | **19.2x** |
| pec11r-4x15k-sxxxx.stp | 35.13 s | 2.00 s | **17.6x** |
| XF2M_6015_1AH.step | 8.58 s | 4.83 s | 1.8x |
| M-MUSBR-A511-R0-REVT1.STEP | 7.57 s | 2.24 s | 3.4x |

The actual HLR phase (post-meshing) now lands in the 5–135 ms range across
the whole corpus. On the heaviest model the 1.1 phase breakdown is
`read 1.68 s / mesh 383 ms / hlr 93 ms / ext 33 ms` — STEP read dominates,
and HLR itself is comfortably interactive.

Detail/simple segment counts differ between modes (poly tessellates curves
into line segments) — this is expected and documented in plan 004. Downstream
clients consume the same `geometry.projection.a0` JSON schema either way.

## Methodology

- These historical results were generated against both a temporary v0.1.0 WASM
  baseline snapshot and the then-current poly HLR build. The checked-in
  `dist/baseline/` artifacts were removed after the performance work completed.
- The current `scripts/bench_hlr.js` is now a current-build timing tool, not a
  checked-in baseline comparison tool.
- Each invocation spawns a fresh Node process running the WASM CLI on one
  STEP, projects the `top` view, writes JSON to a temp file, parses it, and
  records wall time + the new per-phase timings from the current JSON.
- Wall time includes Node startup + module init + STEP read + HLR + JSON
  write. The phase columns (`mesh`, `hlr`, `extract`) are the native
  timings recorded inside `step_hlr_projection_from_bytes`.

Per-model wall-clock time (Node startup + module init + HLR + JSON write).
Each backend's CLI uses its own library defaults: baseline = HLRBRep_Algo
(exact); current = HLRBRep_PolyAlgo with mesh@0.01 mm.

| Model | STEP | Baseline wall | 1.1 wall | Speedup | 1.1 mesh | 1.1 hlr | 1.1 extract | det B / 1.1 | sim B / 1.1 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| _ _ _N101BFCN-RC.STEP | 2658 KB | 2.40 s | 2.22 s | 1.08x | 388 ms | 63 ms | 5 ms | 574 / 134 | 564 / 144 |
| 1437164.stp | 316 KB | 1.49 s | 1.09 s | 1.37x | 167 ms | 38 ms | 73 ms | 172 / 538 | 585 / 352 |
| 1439942.stp | 506 KB | 2.05 s | 1.33 s | 1.54x | 199 ms | 40 ms | 175 ms | 349 / 977 | 827 / 796 |
| 1552269.stp | 514 KB | 2.59 s | 1.23 s | 2.11x | 194 ms | 40 ms | 105 ms | 1564 / 829 | 1423 / 559 |
| ABM3B.STEP | 621 KB | 939 ms | 855 ms | 1.10x | 86 ms | 15 ms | 2 ms | 13 / 23 | 33 / 20 |
| BGA 256 17x17mm 1mm pitch.step | 2362 KB | 3.55 s | 2.73 s | 1.30x | 747 ms | 111 ms | 8 ms | 36 / 164 | 756 / 180 |
| BGA90-8X13mm.step | 1014 KB | 23.96 s | 1.25 s | 19.23x | 214 ms | 42 ms | 4 ms | 448 / 100 | 577 / 106 |
| Cap_SMT_Aluminum_F.STEP | 157 KB | 869 ms | 685 ms | 1.27x | 63 ms | 14 ms | 24 ms | 193 / 456 | 226 / 139 |
| ct-sot-23-5.stp | 102 KB | 650 ms | 630 ms | 1.03x | 38 ms | 8 ms | 4 ms | 298 / 124 | 18 / 18 |
| DO-214AA SMB.STEP | 127 KB | 637 ms | 592 ms | 1.08x | 24 ms | 5 ms | 3 ms | 158 / 56 | 100 / 42 |
| DO-214AB SMC.STEP | 127 KB | 633 ms | 599 ms | 1.06x | 25 ms | 5 ms | 3 ms | 158 / 56 | 100 / 42 |
| DO-214AC SMA.STEP | 125 KB | 648 ms | 619 ms | 1.05x | 27 ms | 5 ms | 3 ms | 158 / 56 | 100 / 42 |
| ecx-306x.stp | 118 KB | 780 ms | 661 ms | 1.18x | 38 ms | 10 ms | 6 ms | 360 / 146 | 245 / 64 |
| FSM4JSMA.step | 673 KB | 1.24 s | 817 ms | 1.52x | 60 ms | 13 ms | 25 ms | 933 / 410 | 894 / 394 |
| FTSH-105-01-X-DV-K-A.step | 560 KB | 910 ms | 850 ms | 1.07x | 73 ms | 12 ms | 4 ms | 105 / 85 | 130 / 66 |
| IHLP2020CZ.STEP | 320 KB | 755 ms | 703 ms | 1.07x | 40 ms | 9 ms | 6 ms | 200 / 108 | 177 / 88 |
| JDEC_153BallWFBGA_eMMC.step | 2443 KB | 47.55 s | 2.31 s | 20.61x | 383 ms | 93 ms | 33 ms | 1763 / 457 | 1877 / 277 |
| miniature_test_point.stp | 64 KB | 630 ms | 599 ms | 1.05x | 33 ms | 7 ms | 3 ms | 16 / 64 | 191 / 64 |
| M-MUSBR-A511-R0-REVT1.STEP | 2101 KB | 7.57 s | 2.24 s | 3.37x | 325 ms | 55 ms | 348 ms | 3720 / 1632 | 2716 / 1218 |
| MSOP8.STEP | 375 KB | 774 ms | 737 ms | 1.05x | 54 ms | 11 ms | 3 ms | 30 / 64 | 150 / 64 |
| pec11r-4x15k-sxxxx.stp | 941 KB | 35.13 s | 2.00 s | 17.54x | 278 ms | 52 ms | 469 ms | 3771 / 1577 | 1690 / 697 |
| RESC1608X06L.step | 70 KB | 535 ms | 555 ms | 0.96x | 21 ms | 5 ms | 3 ms | 10 / 10 | 8 / 8 |
| RESC1608X06N.step | 70 KB | 565 ms | 551 ms | 1.03x | 18 ms | 5 ms | 4 ms | 10 / 10 | 8 / 8 |
| RESC3216X07L.step | 70 KB | 567 ms | 561 ms | 1.01x | 19 ms | 7 ms | 4 ms | 10 / 10 | 8 / 8 |
| SOD-123.step | 483 KB | 948 ms | 818 ms | 1.16x | 62 ms | 13 ms | 11 ms | 529 / 190 | 515 / 136 |
| SOD323-1.step | 98 KB | 600 ms | 571 ms | 1.05x | 25 ms | 5 ms | 4 ms | 207 / 87 | 192 / 72 |
| SOIC-20-300.STEP | 189 KB | 1.18 s | 709 ms | 1.67x | 34 ms | 28 ms | 25 ms | 1173 / 388 | 137 / 77 |
| SOIC-8-W.step | 375 KB | 806 ms | 742 ms | 1.09x | 51 ms | 10 ms | 4 ms | 27 / 69 | 186 / 71 |
| sot223.stp | 86 KB | 633 ms | 597 ms | 1.06x | 28 ms | 6 ms | 3 ms | 301 / 91 | 6 / 6 |
| SOT-23.STEP | 141 KB | 628 ms | 593 ms | 1.06x | 28 ms | 6 ms | 4 ms | 33 / 77 | 209 / 74 |
| SW3DPS-SOT-23-DEFAULT.STEP | 120 KB | 668 ms | 624 ms | 1.07x | 26 ms | 6 ms | 3 ms | 192 / 64 | 102 / 34 |
| TI_DGN_MD.step | 383 KB | 2.35 s | 741 ms | 3.17x | 51 ms | 23 ms | 35 ms | 1474 / 463 | 998 / 362 |
| TSOT-23-5.STEP | 549 KB | 1.63 s | 954 ms | 1.71x | 105 ms | 23 ms | 23 ms | 1568 / 376 | 259 / 137 |
| User Library-SOIC-8.STEP | 428 KB | 917 ms | 777 ms | 1.18x | 65 ms | 13 ms | 8 ms | 513 / 182 | 118 / 55 |
| User Library-SOT23-6.step | 353 KB | 874 ms | 759 ms | 1.15x | 54 ms | 13 ms | 15 ms | 796 / 288 | 742 / 254 |
| XF2M_6015_1AH.step | 6228 KB | 8.58 s | 4.83 s | 1.77x | 946 ms | 135 ms | 293 ms | 1912 / 1912 | 745 / 745 |

## Aggregates

- Baseline total: 157.22 s across 36 models
- 1.1 total: 39.13 s across 36 models
- Mean wall baseline: 4.37 s
- Mean wall 1.1: 1.09 s
- Overall speedup: 4.02x
