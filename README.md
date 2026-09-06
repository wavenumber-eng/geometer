# Geometer

Geometer is a generic C++ geometry library with an executable, Python package,
and browser WASM interfaces. It provides STEP conversion, hidden-line projection
(HLR), planar geometry, and mesh illustration. Board/domain policy and
application rendering decisions belong to callers, not the geometry kernel.

## Capabilities And Maturity

| Capability | Use and maturity |
| --- | --- |
| STEP bounds, GLB conversion, HLR, planar STEP synthesis | Supported geometry APIs. Model bounds has promoted generated contracts; HLR's generated contracts remain pilots. |
| Clipper2 Boolean/offset, planar batch solve and triangulation | Supported polygonized planar workflows; packed interfaces remain handwritten. |
| Fast vector HLR, colored STEP tessellation and mesh illustration | Browser APIs plus direct C++, native IPC, Rust and Python illustration since 2026.9.5; see the [STEP-to-SVG guide](docs/design/mesh-illustration-native.md). Demo applications are evaluation tools, not production renderers. |
| Analytic line/arc planar Boolean | Experimental, not production-ready. May fail closed on valid inputs. Not the dependable whole-board/layer copper-union path; prefer Clipper2 when polygonized output is suitable. |
| Persistent STEP-topology sessions | Experimental native-only subset. Structural declarations do not imply runtime availability. |

Contract promotion and solver production readiness are separate. See
[contract authority](docs/contracts/README.md) and
[the analytic decision](docs/geometer/adr/geometer-adr-017-retain_analytic_planar_boolean_as_experimental.md).

## Choose An Interface

| Need | Entry point |
| --- | --- |
| Keep one executable running; send bytes and receive structured results | [Persistent executable IPC](docs/design/executable-ipc.md): Python, Node/TypeScript and Rust clients |
| Convenient Python file/model operations | [Python package](docs/design/python-package.md) |
| File conversion or scripting | [CLI commands](docs/design/cli.md) |
| Geometry in a browser or Worker | [WASM](docs/design/wasm.md) and [TypeScript clients](docs/design/typescript-client.md) |
| Embed in native C++ | [STEP](docs/design/step-geometry.md) and [planar APIs](docs/design/planar-geometry.md) |
| Manage byte buffers across a foreign-function boundary | [C ABI](docs/design/c-abi.md) and [generic operations](docs/design/generic-operation-c-abi.md) |

Not every operation is exposed through IPC. Check the executable's negotiated
catalog and the [generated operation reference](docs/generated/contracts/index.html).

## Install And Try

Install the Python distribution (import name: `geometer`):

```sh
python -m pip install wn-geometer
geometer --version
```

Convert a STEP file with the executable-backed Python API:

```python
from pathlib import Path
import geometer

Path("part.glb").write_bytes(geometer.model_to_glb(Path("part.step")))
```

Wheels bundle the native executable and install a `geometer` console command.
For persistent processes, use the [IPC quick start](docs/design/executable-ipc.md).
Pin the package version in your application's dependency lock for repeatability.

Source checkouts provide native artifacts under `dist/native/<platform>/`,
browser WASM under `dist/wasm/browser/`, and the generated ESM package under
`dist/wasm/npm/geometer/`. The ESM artifact is not a claim of npm publication.

## Documentation And Examples

[Browse the generated HTML documentation](docs/generated/contracts/guides.html)
or use the authored Markdown sources below.

- [Interface specifications](docs/design/README.md)
- [TypeSpec, contract authority and generated reference](docs/contracts/README.md)
- [Examples and demo status](examples/README.md)
- [Research and historical evidence](docs/research/README.md)
- [Requirements](docs/geometer/requirements/README.md) and [decisions](docs/geometer/adr/README.md)
- [Developer setup, builds, tests and releases](docs/developer/README.md)
- [Release notes](docs/releases/README.md)

Start with the headless Python example or HLR Lab. Analytic and topology demos
remain experimental; retention does not imply production support. See the
[demo audit](docs/developer/demo-status.md) for verification limits.

## Contributing And License

See the developer guide for native/WASM builds and required release checks.
OCCT is generated dependency state under `.deps/`; documentation work does
not require rebuilding it. Releases follow
[date-based versioning](docs/geometer/adr/geometer-adr-006-date_based_versioning_policy.md).

MIT. See [LICENSE](LICENSE).
