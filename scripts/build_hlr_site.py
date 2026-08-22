#!/usr/bin/env python3
"""Build the single-HTML browser HLR lab and its static review directory."""

from __future__ import annotations

from build_self_contained_hlr_demo import OUT, main as build_self_contained_hlr_demo
from package_single_html_site import package_single_html_site


OUTPUT = OUT.parent / "hlr"


def main() -> None:
    build_self_contained_hlr_demo()
    manifest = package_single_html_site(OUT, OUTPUT)
    print(f"Built {OUTPUT} (one runtime file, {manifest['sha256']})")


if __name__ == "__main__":
    main()
