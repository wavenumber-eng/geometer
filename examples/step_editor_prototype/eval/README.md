# eval/ — development tooling

Internal scoring, benchmarking, and training scripts used while building the
auto-conditioner. **These are not part of a fresh checkout's runnable surface** —
they operate on the local reference corpus (`TEST_STEP_FILES/`,
`REFERENCE_STEP_FILES/`, trained `*.joblib` models), all of which are
git-ignored.

Run every script **from the prototype root** (`../`, the folder with
`step_editor.py`) so it can import `app/` and resolve the data dirs:

```bash
cd examples/step_editor_prototype
uv run python eval/score_zsit.py        # AUTO Z-sit vs hand REF + orthogonality gate
uv run python eval/score_pdet.py        # pin-detection audit vs ground truth
uv run python eval/score_auto.py        # per-dimension AUTO-vs-REF scorecard
uv run python eval/benchmark_split.py   # unibody split-quality benchmark
uv run python eval/train_seat_model.py  # (re)train the seat-level ML ranker
uv run python eval/autogen_refs.py      # auto-seat passives into references
uv run python eval/snap_eval.py         # vertex-plane snap evaluation
```

`altium_bake.py` is special: it runs in `toolz/altium_monkey`'s own venv (its
OCP pin differs from this prototype's). See the file header for the invocation.

`scratch/` holds throwaway triage drivers kept only for reference.
