"""Editor settings that ship with the editor: one JSON file next to it
(editor_settings.json), so a copied editor carries its settings along.

Holds the automatic steps, each tool's parameter defaults, the ML model chosen
for each step, the keybinds, and corpus mode (the gold-corpus extras: step
status marks and the Skip button). Per-user conveniences (last folder, panel
width) stay in QSettings."""

from __future__ import annotations

import copy
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
SETTINGS_PATH = HERE / "editor_settings.json"

# action id -> (label, default key sequence)
DEFAULT_KEYBINDS: dict[str, tuple[str, str]] = {
    "next_tool": ("Next tool", "Tab"),
    "prev_tool": ("Previous tool", "Ctrl+Tab"),
    "next_tool_arrow": ("Next tool (arrow)", "Right"),
    "prev_tool_arrow": ("Previous tool (arrow)", "Left"),
    "apply_staged": ("Apply staged changes (stay in the tool)", "Return"),
    "confirm": ("CONFIRM: finish the tool, go to the next", ""),
    "deny": ("Deny staged changes", "Escape"),
    "undo": ("Undo", "Ctrl+Z"),
    "redo": ("Redo", "Ctrl+Y"),
    "write_new": ("Write AP242 (new file)", "Ctrl+S"),
    "write_replace": ("Write AP242 (replace original)", "Ctrl+Shift+S"),
    "open": ("Open model", "Ctrl+O"),
    "next_model": ("Next 3D model", "Ctrl+N"),
    "auto": ("Run AUTO for this tool", "Ctrl+A"),
}

# Candidate ML models per step, from the pipeline plan. The starred one is the
# model designed to run that step; the user may pick another in Settings.
# Trained weights land in models/<step>/<name>.* when training is done.
STEP_MODELS: dict[str, list[str]] = {
    "repair": ["Random Forest (health) + rules", "AdaBoost (health) + rules", "Rules only"],
    "identity": ["Random Forest", "kNN", "HistGradientBoosting"],
    "zsit": ["HistGradientBoosting (candidate ranking)", "Random Forest", "Gaussian process (offset)", "Keras ranker"],
    "detect_pins": ["Random Forest (face groups)", "SVM (face groups)", "Keras MLP"],
    "front": ["Rules (IPC) + Decision tree", "Rules (IPC) + Random Forest"],
    "separate": ["Color split + growth (rules)", "Random Forest (face pairs)", "SVM (face pairs)"],
    "hitboxes": ["Rules (hull of each pin)"],
    "pin_functions": ["Naive Bayes (pin-name n-grams)", "Logistic regression (pin-name n-grams)", "Keras GRU"],
    "footprint_embed": ["IPC rules + CNN (pad shape)", "IPC rules only"],
    "colors": ["Random Forest (face color class)", "kNN", "K-means palette only"],
}

DEFAULTS: dict = {
    "auto_steps": ["repair", "pin_functions", "footprint_embed"],
    "tool_defaults": {},  # tool id -> {widget attribute: value}
    "step_models": {step: {"selected": models[0], "starred": models[0]} for step, models in STEP_MODELS.items()},
    "keybinds": {action: key for action, (_label, key) in DEFAULT_KEYBINDS.items()},
    "corpus_mode": True,
    "live_predict": True,  # Detect Pins: each typed number re-predicts the untyped pins
}


def load() -> dict:
    data = copy.deepcopy(DEFAULTS)
    try:
        saved = json.loads(SETTINGS_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return data
    for key, value in saved.items():
        if isinstance(value, dict) and isinstance(data.get(key), dict):
            data[key].update(value)
        else:
            data[key] = value
    return data


def save(data: dict) -> None:
    try:
        tmp = SETTINGS_PATH.with_suffix(".tmp")
        tmp.write_text(json.dumps(data, indent=2), encoding="utf-8")
        tmp.replace(SETTINGS_PATH)
    except OSError:
        pass  # read-only install: settings last for this session only


METRICS_PATH = HERE / "models" / "metrics.json"
# what the Models page reports per trained model; filled in by training runs
METRIC_COLUMNS = [
    ("f1", "F1"), ("precision", "Precision"), ("recall", "Recall"), ("accuracy", "Accuracy"),
    ("test_error", "Test error"), ("eval_ms", "Avg eval (ms / model)"), ("trained_on", "Trained on"),
    ("trained", "Trained"),
]


def model_metrics() -> dict:
    """{step: {model name: {metric: value}}} from models/metrics.json (written
    by the training runs); empty until a model is trained."""
    try:
        return json.loads(METRICS_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}


def step_model(data: dict, step: str) -> str | None:
    """The ML model that runs `step`, or None when the step has none."""
    if step not in STEP_MODELS:
        return None
    return (data.get("step_models", {}).get(step) or {}).get("selected") or STEP_MODELS[step][0]
