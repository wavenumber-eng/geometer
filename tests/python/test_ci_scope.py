from pathlib import Path
import sys


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from scripts.ci_scope import classify, is_presentation_only  # noqa: E402


def test_documentation_and_html_rebuilds_skip_expensive_ci() -> None:
    paths = [
        "docs/developer/ci-strategy.md",
        "docs/generated/contracts/index.html",
        "docs/generated/contracts/site-manifest.a0.json",
        "dist/wasm/demos/illustration_demo.html",
    ]
    assert all(is_presentation_only(path) for path in paths)
    assert classify(paths) == {
        "docs_only": True,
        "material": False,
        "native": False,
        "python": False,
        "rust": False,
        "typescript": False,
        "wasm": False,
    }


def test_python_change_does_not_build_native_or_wasm() -> None:
    scope = classify(["python/geometer/_ipc.py"])
    assert scope["material"] is True
    assert scope["python"] is True
    assert scope["native"] is False
    assert scope["wasm"] is False
    assert scope["rust"] is False
    assert scope["typescript"] is False


def test_cpp_change_selects_native_and_wasm_once() -> None:
    scope = classify(["src/cpp/lib/model_illustration_operation.cpp"])
    assert scope["native"] is True
    assert scope["wasm"] is True
    assert scope["python"] is False
    assert scope["rust"] is False
    assert scope["typescript"] is False


def test_language_changes_remain_independent() -> None:
    assert classify(["src/rust/geometer-client/src/lib.rs"])["rust"] is True
    assert classify(["src/ts/geometer/index.ts"])["typescript"] is True


def test_contract_and_dependency_changes_cover_native_and_wasm() -> None:
    contract = classify(["src/tsp/geometer/operations.tsp"])
    assert contract["native"] is True
    assert contract["wasm"] is True
    assert contract["python"] is False
    assert contract["rust"] is False
    assert contract["typescript"] is False

    dependency = classify([".github/workflows/occt-deps.yml"])
    assert dependency["native"] is True
    assert dependency["wasm"] is True


def test_ci_router_change_runs_python_validation() -> None:
    scope = classify(["scripts/ci_scope.py"])
    assert scope["python"] is True
    assert scope["native"] is False


def test_manual_run_is_full_validation() -> None:
    scope = classify([], force_full=True)
    assert all(scope[key] for key in ("material", "native", "python", "rust", "typescript", "wasm"))
