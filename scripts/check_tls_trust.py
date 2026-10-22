"""Read-only checks for the build tools affected by the October 2026 CA incident."""

from __future__ import annotations

import importlib.metadata
import json
import shutil
import ssl
import subprocess
from pathlib import Path


AFFECTED_ROOTS = {"emSign ECC Root CA - G3", "AAA Certificate Services"}


def require_uv() -> str:
    command = shutil.which("uv")
    if command is None:
        raise RuntimeError("uv >=0.13.4 is required")
    version = subprocess.check_output([command, "--version"], text=True).split()[1]
    if tuple(map(int, version.split("."))) < (0, 13, 4):
        raise RuntimeError(f"uv >=0.13.4 is required; found {version}")
    return command


def require_node() -> str:
    command = shutil.which("node")
    package = json.loads((Path(__file__).resolve().parents[1] / "package.json").read_text(encoding="utf-8"))
    expected = package["engines"]["node"]
    if command is None:
        raise RuntimeError(f"Node {expected} is required")
    version = subprocess.check_output([command, "--version"], text=True).strip().removeprefix("v")
    if version != expected:
        raise RuntimeError(f"Node {expected} is required; found {version}")
    return command


def affected_roots(context: ssl.SSLContext) -> list[str]:
    names = {
        value
        for certificate in context.get_ca_certs()
        for rdn in certificate["subject"]
        for key, value in rdn
        if key == "commonName"
    }
    if not names:
        raise ValueError("Certificate store has no usable CA roots")
    return sorted(AFFECTED_ROOTS & names)


def main() -> int:
    import certifi

    errors = []
    bundle = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    bundle.load_verify_locations(certifi.where())
    result = {
        "certifi": certifi.__version__,
        "certifi_affected_roots": affected_roots(bundle),
        "system_affected_roots": affected_roots(ssl.create_default_context()),
    }
    if tuple(map(int, certifi.__version__.split("."))) < (2026, 10, 19):
        errors.append("certifi must be >=2026.10.19")
    if result["certifi_affected_roots"] or result["system_affected_roots"]:
        errors.append("An affected CA root remains trusted")
    try:
        pip_version = importlib.metadata.version("pip")
    except importlib.metadata.PackageNotFoundError:
        pip_version = None
    result["pip"] = pip_version
    if pip_version is not None and tuple(map(int, pip_version.split("."))) < (26, 2, 2):
        errors.append("pip must be >=26.2.2 when installed")
    for name, check in (("uv", require_uv), ("node", require_node)):
        try:
            command = check()
            result[name] = subprocess.check_output([command, "--version"], text=True).strip()
        except RuntimeError as error:
            errors.append(str(error))
    result["errors"] = errors
    print(json.dumps(result, indent=2))
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
