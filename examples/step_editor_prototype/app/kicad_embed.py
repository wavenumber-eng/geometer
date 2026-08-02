"""Read and write KiCad 9 `(embedded_files ...)` payloads — our own bake system.

KiCad's on-disk embedded-file format (verified against the KiCad 9 source,
``common/embedded_files.cpp`` + ``libs/kimath/include/mmh3_hash.h``):

  raw bytes -> ZSTD_compress level 15 -> base64, wrapped at 76 chars in |...|
  checksum  -> KiCad's MurmurHash3-128 variant of the RAW (decompressed)
               bytes, seed 0xABBA2345, rendered as 32 uppercase hex chars
               (h1 then h2, each %016X)

KiCad's MMH3 is NOT standard MurmurHash3_x64_128: `MMH3_HASH::addData()`
zero-pads the final partial block to a 4-byte boundary and counts the padding
in `len`, so a stock murmur3 implementation will not reproduce its digests.
`kicad_mmh3_hex()` below is a faithful port of the C++ class.

The bake path deliberately treats the .kicad_mod as text and the payload as
opaque bytes: nothing outside the target `(file ...)` node is touched, which
is what makes the DESIGN_INTENT identity checks ([3]==[5]) achievable.
"""

from __future__ import annotations

import base64
import re
from dataclasses import dataclass

import zstandard

KICAD_EMBED_SEED = 0xABBA2345
KICAD_ZSTD_LEVEL = 15
MIME_BASE64_LENGTH = 76

_U64 = 0xFFFFFFFFFFFFFFFF
_C1 = 0x87C37B91114253D5
_C2 = 0x4CF5AD432745937F


def _rotl64(x: int, r: int) -> int:
    return ((x << r) | (x >> (64 - r))) & _U64


def _fmix64(k: int) -> int:
    k ^= k >> 33
    k = (k * 0xFF51AFD7ED558CCD) & _U64
    k ^= k >> 33
    k = (k * 0xC4CEB9FE1A85EC53) & _U64
    k ^= k >> 33
    return k


def kicad_mmh3_hex(data: bytes, seed: int = KICAD_EMBED_SEED) -> str:
    """KiCad's MMH3_HASH(seed).add(data).digest().ToString() — exact port."""
    h1 = seed
    h2 = seed
    length = 0  # uint32 in C++

    n_blocks = len(data) // 16
    for i in range(n_blocks):
        k1 = int.from_bytes(data[i * 16 : i * 16 + 8], "little")
        k2 = int.from_bytes(data[i * 16 + 8 : i * 16 + 16], "little")

        k1 = (k1 * _C1) & _U64
        k1 = _rotl64(k1, 31)
        k1 = (k1 * _C2) & _U64
        h1 ^= k1
        h1 = _rotl64(h1, 27)
        h1 = (h1 + h2) & _U64
        h1 = (h1 * 5 + 0x52DCE729) & _U64

        k2 = (k2 * _C2) & _U64
        k2 = _rotl64(k2, 33)
        k2 = (k2 * _C1) & _U64
        h2 ^= k2
        h2 = _rotl64(h2, 31)
        h2 = (h2 + h1) & _U64
        h2 = (h2 * 5 + 0x38495AB5) & _U64

        length = (length + 16) & 0xFFFFFFFF

    # Tail: KiCad zero-pads the remainder to a 4-byte boundary and counts the
    # padding in `len` (mmh3_hash.h addData) — this is the non-standard part.
    remaining = len(data) - n_blocks * 16
    tail = data[n_blocks * 16 :]
    if remaining > 0:
        padding = 4 - (remaining + 4) % 4
        tail = tail + b"\x00" * padding
        length = (length + remaining + padding) & 0xFFFFFFFF

    k1 = 0
    k2 = 0
    t = length & 15
    # hashTail switches on len & 15 (post-padding), reading from the padded
    # buffer; mirror the fallthrough ladder with little-endian accumulation.
    if t > 8:
        k2 = int.from_bytes(tail[8:t].ljust(8, b"\x00"), "little")
        k2 = (k2 * _C2) & _U64
        k2 = _rotl64(k2, 33)
        k2 = (k2 * _C1) & _U64
        h2 ^= k2
        t = 8
    if 0 < t <= 8:
        k1 = int.from_bytes(tail[0:t].ljust(8, b"\x00"), "little")
        k1 = (k1 * _C1) & _U64
        k1 = _rotl64(k1, 31)
        k1 = (k1 * _C2) & _U64
        h1 ^= k1

    h1 ^= length
    h2 ^= length
    h1 = (h1 + h2) & _U64
    h2 = (h2 + h1) & _U64
    h1 = _fmix64(h1)
    h2 = _fmix64(h2)
    h1 = (h1 + h2) & _U64
    h2 = (h2 + h1) & _U64

    return f"{h1:016X}{h2:016X}"


@dataclass
class EmbeddedFileEntry:
    """One `(file ...)` node located inside `(embedded_files ...)`."""

    name: str
    file_type: str
    checksum: str
    data_base64: str  # concatenated base64 (| and whitespace stripped)
    span: tuple[int, int]  # [start, end) byte offsets of the (file ...) node


def checksum_matches(checksum: str, raw: bytes) -> bool:
    """Validate either scheme KiCad accepts: 32-char MMH3 or legacy 64-char SHA256."""
    if len(checksum) == 64:
        import hashlib

        return hashlib.sha256(raw).hexdigest() == checksum.lower()
    return kicad_mmh3_hex(raw) == checksum.upper()


def decode_payload(data_base64: str) -> bytes:
    """base64 -> zstd decompress -> raw embedded bytes."""
    compressed = base64.b64decode(data_base64)
    return zstandard.ZstdDecompressor().decompress(compressed)


def encode_payload(raw: bytes) -> tuple[str, str]:
    """raw bytes -> (base64 of zstd-15 frame, KiCad checksum of raw)."""
    compressed = zstandard.ZstdCompressor(
        level=KICAD_ZSTD_LEVEL, write_content_size=True
    ).compress(raw)
    return base64.b64encode(compressed).decode("ascii"), kicad_mmh3_hex(raw)


def _node_span(text: str, open_paren: int) -> tuple[int, int]:
    """Span of one balanced s-expression node starting at `(`; quote-aware."""
    depth = 0
    i = open_paren
    in_string = False
    while i < len(text):
        c = text[i]
        if in_string:
            if c == "\\":
                i += 1
            elif c == '"':
                in_string = False
        elif c == '"':
            in_string = True
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return open_paren, i + 1
        i += 1
    raise ValueError("unbalanced s-expression")


def find_embedded_files(text: str) -> list[EmbeddedFileEntry]:
    """Locate every `(file ...)` entry under `(embedded_files ...)` in `text`."""
    entries: list[EmbeddedFileEntry] = []
    for container in re.finditer(r"\(embedded_files\b", text):
        c_start, c_end = _node_span(text, container.start())
        pos = c_start + len("(embedded_files")
        while True:
            m = re.compile(r"\(file\b").search(text, pos, c_end)
            if not m:
                break
            f_start, f_end = _node_span(text, m.start())
            node = text[f_start:f_end]
            name = re.search(r'\(name\s+"((?:[^"\\]|\\.)*)"', node)
            ftype = re.search(r"\(type\s+([a-z_]+)\s*\)", node)
            checksum = re.search(r'\(checksum\s+"?([0-9A-Fa-f]+)"?\s*\)', node)
            data = re.search(r"\(data\s+\|([^)]*)\|\s*\)", node, re.DOTALL)
            if name and data:
                entries.append(
                    EmbeddedFileEntry(
                        name=name.group(1),
                        file_type=ftype.group(1) if ftype else "",
                        checksum=checksum.group(1) if checksum else "",
                        data_base64=re.sub(r"[\s|]", "", data.group(1)),
                        span=(f_start, f_end),
                    )
                )
            pos = f_end
    return entries


def _data_block(node: str) -> tuple[int, int, int, int]:
    """Spans of the `(data ...)` node and its |...| interior, within `node`."""
    m = re.search(r"\(data\b", node)
    if not m:
        raise ValueError("(file ...) node has no (data ...) block")
    d_start, d_end = _node_span(node, m.start())
    first_bar = node.index("|", d_start)
    last_bar = node.rindex("|", first_bar + 1, d_end)
    return d_start, d_end, first_bar + 1, last_bar


def replace_embedded_payload(text: str, entry: EmbeddedFileEntry, raw: bytes) -> str:
    """Bake `raw` into `entry`'s (file ...) node — our own bake-back system.

    Only the base64 interior of `(data |...|)` and the `(checksum "...")`
    value are rewritten; every other byte of the file is preserved, so baking
    the same payload twice is deterministic and the DESIGN_INTENT identity
    checks ([3]==[5]) hold. EOL style and continuation indentation are taken
    from the file itself.
    """
    f_start, f_end = entry.span
    node = text[f_start:f_end]
    _, _, int_start, int_end = _data_block(node)

    eol = "\r\n" if "\r\n" in node else "\n"
    old_interior = node[int_start:int_end]
    nl = old_interior.find("\n")
    if nl >= 0:
        cont = re.match(r"[\t ]*", old_interior[nl + 1 :]).group(0)
    else:
        # Single-line payload: continuation indent = (data line's indent + one tab.
        line_start = node.rfind("\n", 0, int_start) + 1
        cont = re.match(r"[\t ]*", node[line_start:]).group(0) + "\t"

    b64, checksum = encode_payload(raw)
    chunks = [b64[i : i + MIME_BASE64_LENGTH] for i in range(0, len(b64), MIME_BASE64_LENGTH)]
    new_interior = (eol + cont).join(chunks)

    node = node[:int_start] + new_interior + node[int_end:]
    node, n_sub = re.subn(
        r'(\(checksum\s+")[0-9A-Fa-f]+(")', rf"\g<1>{checksum}\g<2>", node, count=1
    )
    if n_sub != 1:
        raise ValueError("(file ...) node has no (checksum ...) to rewrite")
    return text[:f_start] + node + text[f_end:]


def bake_step_into_kicad_mod(mod_path: str, step_bytes: bytes, out_path: str, name: str | None = None) -> None:
    """Replace the embedded STEP model of `mod_path` with `step_bytes` -> `out_path`."""
    with open(mod_path, "r", encoding="utf-8", newline="") as fh:
        text = fh.read()
    models = [e for e in find_embedded_files(text) if e.file_type == "model"]
    if not models:
        raise ValueError(f"{mod_path}: no embedded model to replace")
    if name is not None:
        models = [e for e in models if e.name == name]
        if not models:
            raise ValueError(f"{mod_path}: no embedded model named {name!r}")
    text = replace_embedded_payload(text, models[0], step_bytes)
    with open(out_path, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


_SCAFFOLD_VERSION = 20241229
_SCAFFOLD_GENERATOR = "wn3d_step_editor"


def scaffold_kicad_mod(footprint_name: str, model_name: str, step_bytes: bytes) -> str:
    """A minimal KiCad 9 footprint whose only content is the embedded model.

    Used to wrap a bare AP242 (e.g. a conditioned export) into a footprint
    KiCad can open directly. UUIDs are derived from the footprint name so
    regenerating the same scaffold is byte-deterministic.
    """
    import uuid

    def uid(role: str) -> str:
        return str(uuid.uuid5(uuid.NAMESPACE_URL, f"wn3d-scaffold:{footprint_name}:{role}"))

    b64, checksum = encode_payload(step_bytes)
    chunks = [b64[i : i + MIME_BASE64_LENGTH] for i in range(0, len(b64), MIME_BASE64_LENGTH)]
    data_interior = "\r\n\t\t\t\t".join(chunks)

    def prop(key: str, value: str, y: float, layer: str, hide: bool) -> str:
        return (
            f'\t(property "{key}" "{value}"\r\n'
            f"\t\t(at 0 {y} 0)\r\n"
            f'\t\t(layer "{layer}")\r\n'
            + ("\t\t(hide yes)\r\n" if hide else "")
            + f'\t\t(uuid "{uid(key)}")\r\n'
            "\t\t(effects\r\n"
            "\t\t\t(font\r\n"
            "\t\t\t\t(size 1 1)\r\n"
            "\t\t\t\t(thickness 0.15)\r\n"
            "\t\t\t)\r\n"
            "\t\t)\r\n"
            "\t)\r\n"
        )

    return (
        f'(footprint "{footprint_name}"\r\n'
        f"\t(version {_SCAFFOLD_VERSION})\r\n"
        f'\t(generator "{_SCAFFOLD_GENERATOR}")\r\n'
        f'\t(generator_version "9.0")\r\n'
        f'\t(layer "F.Cu")\r\n'
        + prop("Reference", "REF**", -2.0, "F.SilkS", False)
        + prop("Value", footprint_name, 2.0, "F.Fab", False)
        + prop("Datasheet", "", 0, "F.Fab", True)
        + prop("Description", "", 0, "F.Fab", True)
        + "\t(attr board_only exclude_from_pos_files exclude_from_bom)\r\n"
        + "\t(embedded_fonts no)\r\n"
        + "\t(embedded_files\r\n"
        + "\t\t(file\r\n"
        + f'\t\t\t(name "{model_name}")\r\n'
        + "\t\t\t(type model)\r\n"
        + f"\t\t\t(data |{data_interior}|\r\n"
        + "\t\t\t)\r\n"
        + f'\t\t\t(checksum "{checksum}")\r\n'
        + "\t\t)\r\n"
        + "\t)\r\n"
        + f'\t(model "kicad-embed://{model_name}"\r\n'
        + "\t\t(offset\r\n\t\t\t(xyz 0 0 0)\r\n\t\t)\r\n"
        + "\t\t(scale\r\n\t\t\t(xyz 1 1 1)\r\n\t\t)\r\n"
        + "\t\t(rotate\r\n\t\t\t(xyz 0 0 0)\r\n\t\t)\r\n"
        + "\t)\r\n"
        + ")\r\n"
    )


def _scaffold(out_dir: str, step_paths: list[str]) -> int:
    import os

    os.makedirs(out_dir, exist_ok=True)
    ok = True
    for path in step_paths:
        with open(path, "rb") as fh:
            step_bytes = fh.read()
        stem = os.path.splitext(os.path.basename(path))[0]
        model_name = stem + ".step"
        text = scaffold_kicad_mod(stem, model_name, step_bytes)

        entry = find_embedded_files(text)[0]
        raw = decode_payload(entry.data_base64)
        identity = raw == step_bytes and checksum_matches(entry.checksum, raw)
        ok &= identity

        out_path = os.path.join(out_dir, stem + ".kicad_mod")
        with open(out_path, "w", encoding="utf-8", newline="") as fh:
            fh.write(text)
        print(
            f"{stem}.kicad_mod: {len(step_bytes)} -> {os.path.getsize(out_path)} bytes, "
            f"re-extract {'OK' if identity else 'FAIL'}"
        )
    return 0 if ok else 1


def _self_check(paths: list[str]) -> int:
    ok = True
    for path in paths:
        with open(path, "r", encoding="utf-8") as fh:
            text = fh.read()
        entries = find_embedded_files(text)
        if not entries:
            print(f"{path}: NO embedded files found")
            ok = False
            continue
        for e in entries:
            raw = decode_payload(e.data_base64)
            match = checksum_matches(e.checksum, raw)
            ok &= match
            scheme = "sha256-legacy" if len(e.checksum) == 64 else "mmh3"
            print(
                f"{e.name} ({e.file_type}): {len(raw)} bytes raw, "
                f"{scheme} checksum -> {'MATCH' if match else 'MISMATCH'}"
            )
    return 0 if ok else 1


def _bake_test(paths: list[str]) -> int:
    """Bake each file's own payload back into itself and verify the identities."""
    ok = True
    for path in paths:
        with open(path, "r", encoding="utf-8", newline="") as fh:
            text = fh.read()
        models = [e for e in find_embedded_files(text) if e.file_type == "model"]
        if not models:
            print(f"{path}: NO embedded model")
            ok = False
            continue
        entry = models[0]
        raw = decode_payload(entry.data_base64)

        baked = replace_embedded_payload(text, entry, raw)
        entry2 = [e for e in find_embedded_files(baked) if e.file_type == "model"][0]
        raw2 = decode_payload(entry2.data_base64)
        identity = raw2 == raw
        cksum = checksum_matches(entry2.checksum, raw2)

        baked_again = replace_embedded_payload(baked, entry2, raw)
        deterministic = baked_again == baked

        zstd_parity = baked == text  # our zstd-15 frame == KiCad's, byte for byte

        ok &= identity and cksum and deterministic
        print(
            f"{entry.name}: payload-identity {'OK' if identity else 'FAIL'}, "
            f"checksum {'OK' if cksum else 'FAIL'}, "
            f"deterministic {'OK' if deterministic else 'FAIL'}, "
            f"zstd-parity-with-kicad {'YES' if zstd_parity else 'no'}"
        )
    return 0 if ok else 1


if __name__ == "__main__":
    import sys

    args = sys.argv[1:]
    if args and args[0] == "--bake-test":
        raise SystemExit(_bake_test(args[1:]))
    if args and args[0] == "--scaffold":
        raise SystemExit(_scaffold(args[1], args[2:]))
    raise SystemExit(_self_check(args))
