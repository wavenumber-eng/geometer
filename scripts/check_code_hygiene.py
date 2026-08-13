from __future__ import annotations

import argparse
import ast
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent

SCAN_ROOTS = [
    ROOT / "setup.py",
    ROOT / "src",
    ROOT / "python",
    ROOT / "scripts",
    ROOT / "tests",
    ROOT / "examples" / "python",
    ROOT / "examples" / "wasm",
]

CODE_SUFFIXES = {".py", ".cpp", ".h", ".hpp", ".js", ".mjs", ".ts", ".html"}
SKIP_PARTS = {
    ".git",
    ".deps",
    ".venv",
    "__pycache__",
    "build",
    "build-native-linux-x64",
    "build-wsl",
    "build-wasm",
    "dist",
    "out",
    "rack_results",
    "target",
    "third_party",
}

MAX_FILE_LINES = 1500
MAX_PY_FUNCTION_LINES = 180
MAX_PY_CLASS_LINES = 450
MAX_PY_FUNCTION_COMPLEXITY = 25
MAX_PY_CLASS_COMPLEXITY = 35

ALLOWED_DIST_ROOT_FILES = {".gitkeep", "README.md"}
ALLOWED_DIST_ROOT_DIRS = {"native", "npm", "wasm"}


@dataclass(slots=True)
class Violation:
    path: Path
    message: str

    def format(self) -> str:
        return f"{self.path.relative_to(ROOT)}: {self.message}"


def should_skip(path: Path) -> bool:
    return any(part in SKIP_PARTS for part in path.relative_to(ROOT).parts)


def iter_code_files() -> list[Path]:
    files: list[Path] = []
    for root in SCAN_ROOTS:
        if not root.exists():
            continue
        if root.is_file():
            if root.suffix.lower() in CODE_SUFFIXES:
                files.append(root)
            continue
        for path in root.rglob("*"):
            if not path.is_file() or should_skip(path):
                continue
            if path.suffix.lower() in CODE_SUFFIXES:
                files.append(path)
    return sorted(files)


def decision_count(node: ast.AST) -> int:
    decision_nodes = (
        ast.If,
        ast.For,
        ast.While,
        ast.Try,
        ast.ExceptHandler,
        ast.BoolOp,
        ast.IfExp,
        ast.comprehension,
        ast.Match,
    )
    return sum(isinstance(child, decision_nodes) for child in ast.walk(node))


def check_file_lengths(files: list[Path]) -> list[Violation]:
    violations: list[Violation] = []
    for path in files:
        line_count = len(path.read_text(encoding="utf-8").splitlines())
        if line_count > MAX_FILE_LINES:
            violations.append(Violation(path, f"{line_count} lines exceeds limit {MAX_FILE_LINES}"))
    return violations


def check_python_shapes(files: list[Path]) -> list[Violation]:
    violations: list[Violation] = []
    for path in files:
        if path.suffix != ".py":
            continue
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        for node in ast.walk(tree):
            if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
                continue
            end_lineno = getattr(node, "end_lineno", None)
            lineno = getattr(node, "lineno", None)
            if not isinstance(end_lineno, int) or not isinstance(lineno, int):
                continue
            length = end_lineno - lineno + 1
            complexity = decision_count(node)
            if isinstance(node, ast.ClassDef):
                if length > MAX_PY_CLASS_LINES:
                    violations.append(
                        Violation(path, f"class {node.name} has {length} lines; limit {MAX_PY_CLASS_LINES}")
                    )
                if complexity > MAX_PY_CLASS_COMPLEXITY:
                    violations.append(
                        Violation(
                            path,
                            f"class {node.name} has complexity {complexity}; limit {MAX_PY_CLASS_COMPLEXITY}",
                        )
                    )
            else:
                if length > MAX_PY_FUNCTION_LINES:
                    violations.append(
                        Violation(path, f"function {node.name} has {length} lines; limit {MAX_PY_FUNCTION_LINES}")
                    )
                if complexity > MAX_PY_FUNCTION_COMPLEXITY:
                    violations.append(
                        Violation(
                            path,
                            f"function {node.name} has complexity {complexity}; limit {MAX_PY_FUNCTION_COMPLEXITY}",
                        )
                    )
    return violations


def check_dist_root() -> list[Violation]:
    dist = ROOT / "dist"
    if not dist.exists():
        return []
    violations: list[Violation] = []
    for child in dist.iterdir():
        if child.is_dir() and child.name not in ALLOWED_DIST_ROOT_DIRS:
            violations.append(Violation(child, "unexpected root-level dist directory"))
        elif child.is_file() and child.name not in ALLOWED_DIST_ROOT_FILES:
            violations.append(Violation(child, "unexpected root-level dist artifact"))
    return violations


def check_removed_working_docs() -> list[Violation]:
    violations: list[Violation] = []
    if (ROOT / "CLAUDE.md").exists():
        violations.append(Violation(ROOT / "CLAUDE.md", "remove Claude-specific repo notes"))
    if (ROOT / "docs" / "plans").exists():
        violations.append(Violation(ROOT / "docs" / "plans", "completed plans should not persist in the repo"))
    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description="Check Geometer release hygiene.")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    files = iter_code_files()
    violations = [
        *check_file_lengths(files),
        *check_python_shapes(files),
        *check_dist_root(),
        *check_removed_working_docs(),
    ]
    if violations:
        for violation in violations:
            print(violation.format())
        return 1
    if not args.quiet:
        print(f"code hygiene passed ({len(files)} files)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
