# Cyclomatic Complexity audit and plan (2026-10-31)

Measured with radon (`--selftest cc`), every `.py` in the prototype except
venvs and fixtures. Cap: every function under CC 10.

## Where it stands

- **123 functions at CC 10 or above** (124 this morning, 96 before the 10-30 overhaul; the
  overhaul added tools faster than it factored them).
- They hold 5,820 lines, the bulk of the editor's logic.
- By band: 7 at CC 30+, 16 at CC 20-29, 31 at CC 15-19, 69 at CC 10-14.
- Worst: `join_mouth_pins` 37, `predict_names` 34, `selftest_m3` 33, `condition_auto` 33, `match_pins_to_pads` 31.

Why it matters here: the same code runs inside `wavenumber_condition_step`
for the ML array, so a branch nobody can follow is a label nobody can trust.
High-CC functions are also where this week's bugs were (coplanar box cuts,
mouth-pin tails, Settings save).

## The gate is now a ratchet

`--selftest cc` used to fail on all of them, so it failed every run and nobody
looked. It now compares against `app/cc_baseline.json` (today's list):

- a **new** function at CC 10+ fails;
- a listed function whose CC **rises** fails;
- a listed function that drops is reported, and `WN3D_CC_REBASE=1
  python step_editor.py --selftest cc` rewrites the baseline smaller.

So the count can only go down. The end state is an empty baseline.

## How to cut CC without changing behaviour

1. **Extract each phase** of a long function into a named helper (most of
   the 30+ functions are 3-5 phases in a row: gather, filter, score, pick).
2. **Tables over if-chains**: kind/field dispatch (`metadata`, `replay`,
   `restore`, `from_json`) becomes a dict of small handlers.
3. **Early returns / guard clauses** instead of nested ifs.
4. **Small dataclasses** for the 6-8 values threaded through a function.
5. **Paint events** draw through one helper per layer (grid, shapes, labels).

Each refactor is behaviour-preserving: the 140 tests and the m0-m8 / auto /
sep selftests must pass before and after, one wave per commit. Functions
without a test get one first (pin the current output on a fixture).

## Waves

| Wave | Area | Functions | Target |
|---|---|---|---|
| 1 | Geometry and pin algorithms | 33 | Nov 14 |
| 2 | Files, metadata, replay | 19 | Nov 21 |
| 3 | Tool panels and drawing | 56 | Dec 5 |
| 4 | Selftests, eval and tests | 15 | Dec 12 (or exempt) |

Wave 1 first: those functions run headless in the pipeline and have the
best test cover. Wave 4 is last because long selftests are long by nature;
they may be split into check helpers or exempted by name in the baseline.
The editor is not on the corpus critical path (freeze Nov 4), so waves fit
around the ML schedule.

### Wave 1: Geometry and pin algorithms

| CC | Function | File:line | Lines |
|---|---|---|---|
| 37 | `join_mouth_pins` | app/pins.py:581 | 135 |
| 33 | `condition_auto` | app/auto.py:967 | 143 |
| 31 | `match_pins_to_pads` | app/pins.py:772 | 68 |
| 31 | `_planar_patches_for_wire` | app/document.py:479 | 48 |
| 30 | `compute_auto_zsit` | app/tools/zsit.py:249 | 156 |
| 28 | `split_by_box` | app/document.py:1573 | 80 |
| 25 | `find_similar_regions` | app/pins.py:450 | 67 |
| 23 | `find_pin_cut` | app/pins.py:939 | 104 |
| 22 | `auto_separate_factor` | app/auto.py:664 | 84 |
| 21 | `split_by_cuts` | app/document.py:1687 | 62 |
| 19 | `respect_coplanar_faces` | app/document.py:1075 | 48 |
| 18 | `color_region_bundles` | app/pins.py:1072 | 49 |
| 18 | `z_fighting` | app/health.py:185 | 43 |
| 17 | `predict_remaining_numbers` | app/pins.py:60 | 22 |
| 16 | `_ear_clip_triangles` | app/document.py:540 | 45 |
| 14 | `detect_pins_by_section` | app/pins.py:1256 | 54 |
| 13 | `_refresh_preview` | app/tools/zsit.py:856 | 61 |
| 13 | `split_face` | app/document.py:2009 | 70 |
| 13 | `auto_logo_placement` | app/auto.py:813 | 93 |
| 12 | `transform_pins` | app/pins.py:718 | 32 |
| 12 | `remap_pin_faces` | app/pins.py:544 | 35 |
| 12 | `_without_voids` | app/document.py:856 | 37 |
| 11 | `on_pick` | app/tools/zsit.py:708 | 28 |
| 11 | `compute_zsit_frame` | app/tools/zsit.py:119 | 48 |
| 11 | `pin_solid_ids` | app/hitbox.py:128 | 20 |
| 11 | `pin_hitbox` | app/hitbox.py:175 | 21 |
| 11 | `disjoint` | app/health.py:230 | 15 |
| 11 | `apply_fix` | app/health.py:408 | 23 |
| 10 | `process_pin` | app/pins.py:1195 | 16 |
| 10 | `link_pin_pieces` | app/pins.py:888 | 20 |
| 10 | `_predict_grid_with_axes` | app/pins.py:1545 | 44 |
| 10 | `shrink_wrap_hitbox` | app/hitbox.py:198 | 24 |
| 10 | `auto_inherit_functions` | app/auto.py:750 | 17 |

### Wave 2: Files, metadata, replay

| CC | Function | File:line | Lines |
|---|---|---|---|
| 26 | `verify_metadata_text` | app/export_ap242.py:212 | 95 |
| 25 | `restore_state` | app/restore.py:83 | 51 |
| 25 | `_replay_separate` | app/replay.py:387 | 57 |
| 24 | `build_model_3d` | app/model3d_export.py:132 | 72 |
| 23 | `from_json` | mock_data_model/model_3d.py:78 | 19 |
| 22 | `build_metadata` | app/metadata.py:40 | 73 |
| 19 | `open_vendor_file` | app/vendor_files.py:166 | 66 |
| 17 | `_separate_link_cut_pins` | app/replay.py:347 | 38 |
| 15 | `from_json` | mock_data_model/model_3d_pin.py:79 | 19 |
| 15 | `write_step` | app/export_ap242.py:332 | 62 |
| 13 | `from_json` | mock_data_model/model_3d_zplane_feature.py:62 | 19 |
| 12 | `to_json` | mock_data_model/model_3d.py:53 | 23 |
| 12 | `_detect_select_group` | app/replay.py:241 | 18 |
| 11 | `sniff_model_format` | app/vendor_files.py:77 | 19 |
| 11 | `_replay_hitboxes` | app/replay.py:448 | 19 |
| 11 | `resolve_entity_refs` | app/model3d_export.py:71 | 30 |
| 10 | `from_json` | mock_data_model/model_3d_hitbox.py:46 | 10 |
| 10 | `inject_metadata` | app/export_ap242.py:160 | 50 |

### Wave 3: Tool panels and drawing

| CC | Function | File:line | Lines |
|---|---|---|---|
| 34 | `predict_names` | app/tools/detect_pins.py:885 | 88 |
| 24 | `geometry_footprint` | app/tools/footprint_embed.py:117 | 43 |
| 21 | `_apply_cuts` | app/tools/separate.py:1495 | 95 |
| 20 | `paintEvent` | app/tools/pin_functions.py:172 | 127 |
| 20 | `_refresh_views` | app/tools/hitboxes.py:427 | 56 |
| 20 | `_detect_similar` | app/tools/detect_pins.py:414 | 62 |
| 19 | `_update_cut_preview` | app/tools/separate.py:1288 | 57 |
| 19 | `_find_matching` | app/tools/separate.py:1037 | 86 |
| 19 | `paintEvent` | app/ortho2d.py:61 | 91 |
| 17 | `_link_cut_pins` | app/tools/separate.py:1388 | 50 |
| 17 | `_show` | app/tools/footprint_embed.py:378 | 30 |
| 17 | `_candidate_groups` | app/tools/detect_pins.py:1096 | 32 |
| 17 | `staged_rows` | app/tools/base.py:89 | 27 |
| 16 | `main` | step_editor.py:22 | 75 |
| 16 | `apply` | app/tools/separate.py:2117 | 83 |
| 16 | `number_from_footprint` | app/tools/detect_pins.py:1026 | 38 |
| 16 | `select_same_color` | app/tools/colors.py:429 | 23 |
| 16 | `_resume_conditioning` | app/app_window.py:1084 | 54 |
| 15 | `_stage_paint_copies` | app/tools/separate.py:432 | 34 |
| 15 | `ipc_land_pattern` | app/tools/footprint_embed.py:170 | 37 |
| 15 | `paintEvent` | app/mode_rect.py:120 | 39 |
| 15 | `compact_panel` | app/compact.py:119 | 31 |
| 14 | `match_pins` | app/tools/footprint_embed.py:46 | 18 |
| 14 | `renumber` | app/tools/detect_pins.py:765 | 38 |
| 14 | `_run_context_plane` | app/tools/detect_pins.py:584 | 43 |
| 14 | `_join_mouth_pins` | app/tools/detect_pins.py:478 | 40 |
| 14 | `_bake_test` | app/kicad_embed.py:386 | 33 |
| 14 | `_reflow` | app/compact.py:72 | 25 |
| 13 | `fix` | app/tools/repair.py:266 | 37 |
| 13 | `_set_function` | app/tools/pin_functions.py:487 | 43 |
| 13 | `on_pick` | app/tools/detect_pins.py:342 | 23 |
| 13 | `show_hitboxes` | app/scene.py:675 | 51 |
| 13 | `eventFilter` | app/scene.py:996 | 24 |
| 13 | `loops_to_regions` | app/dxf_loader.py:92 | 28 |
| 12 | `on_pick` | app/tools/separate.py:608 | 34 |
| 12 | `_chain_loops` | app/tools/footprint_embed.py:66 | 24 |
| 12 | `_click_then_geometric` | app/tools/detect_pins.py:743 | 12 |
| 12 | `select_similar` | app/tools/colors.py:453 | 34 |
| 12 | `_tool_page` | app/app_window.py:372 | 42 |
| 11 | `_record_cuts` | app/tools/separate.py:1469 | 16 |
| 11 | `_build_cut_preview_mesh` | app/tools/separate.py:1716 | 36 |
| 11 | `check` | app/tools/repair.py:218 | 33 |
| 11 | `_auto_one` | app/tools/hitboxes.py:320 | 36 |
| 11 | `_toggle_group` | app/tools/detect_pins.py:1152 | 16 |
| 11 | `_select_pin_from_pick` | app/tools/detect_pins.py:812 | 36 |
| 11 | `_draw_pin_callouts` | app/tools/detect_pins.py:1263 | 37 |
| 11 | `_save` | app/settings_window.py:361 | 19 |
| 11 | `_models_page` | app/settings_window.py:291 | 38 |
| 11 | `_reframe_if_lost` | app/app_window.py:1252 | 25 |
| 10 | `_init_paint_bundles` | app/tools/separate.py:563 | 19 |
| 10 | `repair_solid` | app/tools/repair.py:68 | 32 |
| 10 | `_refresh_views` | app/tools/detect_pins.py:1210 | 24 |
| 10 | `_load_all` | app/refs.py:98 | 30 |
| 10 | `apply_settings` | app/app_window.py:580 | 37 |
| 10 | `_choose_footprint` | app/app_window.py:962 | 48 |

### Wave 4: Selftests, eval and tests

| CC | Function | File:line | Lines |
|---|---|---|---|
| 33 | `selftest_m3` | app/selftest.py:218 | 205 |
| 19 | `selftest_sep` | app/selftest.py:546 | 96 |
| 18 | `main` | eval/benchmark_split.py:102 | 67 |
| 15 | `test_model3d_block_links_and_roundtrips` | tests/test_model3d_contract.py:56 | 1 |
| 15 | `main` | eval/score_zsit.py:220 | 45 |
| 14 | `replace_main` | eval/altium_bake.py:218 | 43 |
| 14 | `selftest_m8` | app/selftest.py:729 | 108 |
| 14 | `selftest_kicad_rt` | app/selftest.py:929 | 109 |
| 13 | `test_tangent_frame_on_non_planar_face` | tests/test_journal_and_pins.py:210 | 1 |
| 13 | `_candidates` | eval/autogen_refs.py:46 | 17 |
| 12 | `run_auto` | eval/benchmark_split.py:45 | 26 |
| 12 | `selftest_m7` | app/selftest.py:663 | 64 |
| 11 | `replace_model_payload` | eval/altium_bake.py:45 | 42 |
| 11 | `main` | eval/altium_bake.py:90 | 39 |
| 10 | `main` | eval/snap_eval.py:26 | 48 |
