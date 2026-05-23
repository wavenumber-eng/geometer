from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class GeometerError(RuntimeError):
    """Error raised when the native Geometer C ABI returns a non-zero code."""

    code: int
    message: str
    function: str
    version: str | None = None
    abi: int | None = None

    def __post_init__(self) -> None:
        details = f"{self.function} failed with code {self.code}: {self.message}"
        if self.version is not None and self.abi is not None:
            details += f" (geometer {self.version}, abi {self.abi})"
        RuntimeError.__init__(self, details)
