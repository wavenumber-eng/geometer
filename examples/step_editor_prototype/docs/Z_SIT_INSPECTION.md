# Z-Sit AUTO inspection — manual verdicts (held-out + training corpus)

Inspected in the 3D browser (orthographic Top/Front views + z=0 grid). Criteria:
**(1) Z-plane level, (2) Z-axis / hull centering, (3) plane on the correct seat faces.**

Legend: ✅ Correct · 🟡 Almost (incl. exact-Z) · ❌ Incorrect.
Fix category drives the next AUTO work.

**Mechanical-truth rule:** the seat plane MUST sit exactly at the PCB-contact
level, because z-height = the part's true standoff above the board. A "tiny"
Z error is still wrong — there is no "functionally correct," only correct.

## Summary
- ✅ Correct: 55 · 🟡 Almost: 9 · ❌ Incorrect: 22
- **Dominant failure = seat LEVEL** (THR shift-up / BGA shift-down / wrong Z offset, incl. the exact-Z standoff cases) → seat-level ranker.
- **Orientation errors (few):** `_ _ _N101BFCN-RC`, `M-MUSBR-A511`, `TSOT-23-5`, `KSZ9896CTXC` (45°), `miniature_test_point`.
- **Hull-centering needed:** `471511051`, `B4B-PH-SM4-TBT`.

---

## ❌ Incorrect

| File | Category | Correction |
|---|---|---|
| 1437164.stp | unspecified | wrong (reason TBD) |
| 1439942.stp | unspecified | wrong (reason TBD) |
| 1552269.stp | unspecified | wrong (reason TBD) |
| _ _ _N101BFCN-RC.STEP | orientation | Z-plane oriented incorrectly |
| B4B-PH-SM4-TBT.step | THR + centering | shift plane to largest bottom plane (up Z); hull-center; orientation OK |
| BGA 256 17x17mm 1mm pitch.step | BGA | plane on the BGA ball tips (shift down Z); orient/center OK |
| BGA90-8X13mm.step | BGA | plane on the ball tips (shift down Z) |
| JDEC_153BallWFBGA_eMMC.step | BGA | plane on the ball tips (shift down Z) |
| BM04B-SRSS-TB.STEP | THR | shift up Z (seat on body, leads below) |
| FTSH-105-01-X-DV-K-A.step | THR/offset | orient+center OK; shift up Z |
| KSZ9896CTXC.STEP | orientation+offset | orientation 45° off about Z; center OK; shift down Z |
| M-MUSBR-A511-R0-REVT1.STEP | orientation | oblique angle in Y — wrong orientation |
| miniature_test_point.stp | seat face | largest bottom face should be on the Z plane |
| pec11r-4x15k-sxxxx.stp | THR | through-hole — seat on body, leads below |
| SOIC-8-W.step | offset | orient+center OK; move Z-plane down |
| SSSS811101.STEP | THR | through-hole — shift up Z |
| step_temp.STEP | THR | through-hole — shift up Z |
| TRJG0926HENL .stp | THR | through-hole — seat on body, leads below |
| TSOT-23-5.STEP | orientation | everything OK except orientation |
| USB3stacked.STEP | offset | shift Z-plane down |
| YZ52015028P-01.STEP | THR | through-hole — shift up Z |
| YZP0120-16020-01.STEP | THR | through-hole — shift up Z |

## 🟡 Almost correct

| File | Category | Correction |
|---|---|---|
| 1711725.STEP | offset | plane coplanar with largest bottom plane; center OK; shift up Z |
| 3x4x2 tact button.step | offset | orient+center OK; shift down Z |
| 471511051.stp | offset + centering | orient OK; bad centering + wrong level — shift up Z, hull-center |
| 90414-15011-21.STEP | offset | center+orient OK; wrong Z offset — shift down Z |
| BM4B-SRSS-TB.step | THR + centering | center + Z shift |
| BM6B-SRSS-TB.step | THR + centering | center + Z shift (same as BM4B) |
| FSM4JSMA.step | offset | orient+center OK; shift down Z |
| IS43TR16256B-125KBL.STEP | exact-Z | orient+center perfect; ~0.01mm Z standoff off — must be exact |
| L_DO5010H_800.STEP | exact-Z | orient+center good; plane not flush with a real feature — snap to it |

## ✅ Correct

0399-T208.STEP · 10164227-1001a1rlf.stp · ABM3B.STEP · ABM8-272-T3.STEP ·
AD5697RBRUZ.STEP · BQ34Z100-R2.STEP · C0603_0.90MM_MD.STEP · C0805_1.00MM_MD.STEP ·
C0805_1.45MM_MD.STEP · C1206_1.80MM_MD.STEP · C1210_2.70MM_MD.STEP ·
C1210_2.80MM_MD.STEP · Cap_SMT_Aluminum_F.STEP · ct-sot-23-5.stp · D_SOD-123_P.STEP ·
D_SOT-23-3_P.STEP · DO-214AA SMB.STEP · DO-214AB SMC.STEP · DO-214AC SMA.STEP ·
ECS-2333-500-BN-TR.STEP · ecx-306x.stp · FD-3RGB45-N6.stp · FH12-22S-0.5SH.stp ·
IHLP2020CZ.STEP · INA226.STEP · MSOP8.STEP · MTSSD03-67MSW337.STEP · PZ254-2-02-S.STEP ·
R0603_0.55MM_HD.STEP · R1206_0.65MM_MD.STEP · R1206_0.70MM_HD.STEP · R2512_MD.STEP ·
RESC1608X06L.step · RESC1608X06N.step · RESC3216X07L.step · SGM61410XN6G_TR.STEP ·
SN74HC164PWR.STEP · SOD-123.step · SOD323-1.step · SOIC-20-300.STEP · SOT-23.STEP ·
SOT1850-1_VFBGA176.step · SOT2137-1_VFBGA112.step · sot223.stp · SOT23-5-5.STEP ·
SOT23-5.STEP · SOT905-1_HVQFN24.step · SRP5030CC.stp · SW3DPS-SOT-23-DEFAULT.STEP ·
TI_DGN_MD.step · User Library-SOIC-8.STEP · User Library-SOT23-6.step ·
WE-RJ45LAN-7498111120AR (rev1).stp · XF2M_6015_1AH.step · YZ153915020R-01.STEP ·
YZ70115093P-02.STEP · YZR0008-15030-01.STEP · YZR0028-15020-01.STEP ·
YZR0090-15038-01.STEP · YZR0154-15039-02.STEP · YZR0180-20025-02.STEP · YZR0188-15026-01.STEP

---

## Raw verdicts (verbatim, as provided)

[039] Good, [101] Good, [1437] Incorrect, [1439] Incorrect, [1552] Incorrect,
[1711] Almost correct (Z-Sit plane coplanar with largest bottom plane; centering good {shift up Z}),
[3x4x2] Almost correct (orientation + centering good {shift down Z}),
[4715] Almost correct (orientation good; centering not good and wrong level {shift up Z, hull center}),
[9041] Almost correct (centered + oriented OK; wrong Z offset {shift down Z}),
[_ _ _N101] Incorrect (Z-Plane oriented incorrectly), [ABM3] Correct, [ABM8] Correct, [AD56] Correct,
[B4B-] Incorrect (shift plane to largest bottom plane; orientation good; centering hull {shift up Z, hull center}),
[BGA ] Incorrect (orientation + centering good; plane on the BGA ball tips {shift down Z}),
[BGA9] Incorrect (same as first BGA), [BM04] Incorrect (through hole {Z up}),
[BM4B] Almost (good except center + z shift), [BM6B] Almost (same as BM4B), [BQ34] Correct,
[C060][C080][C080][C120][C121][C121] correct, [Cap_] Correct, [ct-so] Correct, [D_SOD] Correct,
[D_SOT] Correct, [DO-214AA/AB/AC] correct, [ECS-] Correct, [ecx-] Correct, [FD-3R] Correct,
[FH12] Correct, [FSM4] Almost (orient good, center good {shift down Z}),
[FTSH] Incorrect (orient good, center good {shift up Z}), [IHLP] Correct, [INA2] Correct,
[IS43] Functionally correct (~0.01mm off Z standoff), [JDEC] Incorrect (same as BGA),
[KSZ9] Incorrect (orientation 45° off in Z; center good; shift down Z),
[L_DO5] functionally correct (plane not flush with features; center + orientation good),
[M-MUS] Incorrect (orientation wrong; oblique angle in Y),
[mini] Incorrect (largest bottom face should be on the Z plane), [MSOP] Correct, [MTSS] Correct,
[pec11] Incorrect (through hole), [PZ25] Correct, [R060] Correct,
[R120][R120][R251][RESC][RESC][RESC] correct, [SGM6] Correct, [SN74] Correct, [SOD-] Correct,
[SOD3] Correct, [SOIC] Correct, [SOIC-8-W] Incorrect (all OK except Z-plane sit {move down Z}),
[SOT-] Correct, [SOT1][SOT2][sot2][SOT2][SOT2][SOT9] correct, [SRP5] Correct,
[SSSS] Incorrect (through hole), [step_] Incorrect (through hole), [SW3D] Correct, [TI_DG] Correct,
[TRJG] Incorrect (through hole), [TSOT] Incorrect (all OK except orientation),
[USB3] Incorrect (shift Z-plane down), [User][User] correct, [WE-RJ] Correct, [XF2M] Correct,
[YZ15/70] Correct, [YZ52] Incorrect (through hole), [YZP0] Incorrect (through hole),
[YZR0]x7 all good.
