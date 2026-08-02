"""Batch AUTO conditioner: run the full headless auto chain on every source
3D file in TEST_STEP_FILES and dump the conditioned AP242 into
TEST_STEP_FILES/AUTO_FILTERED_STEP. Each file runs in its own subprocess with
a timeout so one slow/bad model can't stall the batch. Throwaway driver."""

import subprocess
import sys
import time
from pathlib import Path

PROTO = Path(__file__).resolve().parents[2]  # prototype root (script in eval/scratch/)
SRC = PROTO / "TEST_STEP_FILES"
OUT = SRC / "AUTO_FILTERED_STEP"
OUT.mkdir(exist_ok=True)
EXTS = {".step", ".stp"}
TIMEOUT = 900  # seconds per file

files = [
    f for f in sorted(SRC.iterdir())
    if f.is_file()
    and f.suffix.lower() in EXTS
    and "_AP242_conditioned" not in f.stem
]

log = OUT / "_AUTO_BATCH_LOG.txt"
lines = [f"AUTO batch: {len(files)} source files -> {OUT}", ""]
log.write_text("\n".join(lines), encoding="utf-8")

ok = 0
for index, f in enumerate(files, 1):
    dst = OUT / f"{f.stem}_AP242_conditioned.step"
    start = time.perf_counter()
    try:
        proc = subprocess.run(
            [sys.executable, "step_editor.py", "--auto", str(f), "--out", str(dst)],
            capture_output=True, text=True, timeout=TIMEOUT, cwd=str(PROTO),
        )
        dt = time.perf_counter() - start
        out_lines = [ln for ln in proc.stdout.splitlines() if ln.strip()]
        summary = out_lines[-1] if out_lines else "(no output)"
        if proc.returncode == 0 and dst.exists():
            ok += 1
            line = f"OK     {f.name:44} {dt:7.1f}s  {summary}"
        else:
            err = (proc.stderr or "").strip().splitlines()
            line = (f"FAIL   {f.name:44} {dt:7.1f}s  rc={proc.returncode}  "
                    f"{summary}  {err[-1] if err else ''}")
    except subprocess.TimeoutExpired:
        line = f"TIMEOUT{f.name:44} >{TIMEOUT}s"
    except Exception as exc:  # noqa: BLE001
        line = f"ERROR  {f.name:44} {exc}"
    print(f"[{index}/{len(files)}] {line}", flush=True)
    lines.append(line)
    log.write_text("\n".join(lines), encoding="utf-8")

lines += ["", f"DONE: {ok}/{len(files)} conditioned OK -> {OUT}"]
log.write_text("\n".join(lines), encoding="utf-8")
print(f"\nDONE: {ok}/{len(files)} OK", flush=True)
