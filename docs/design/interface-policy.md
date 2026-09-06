# Interface Policy And Versioning

## Interface Policy

Geometer interfaces must describe generic geometry operations. Keep downstream
application concepts such as PCB placement policy, Altium/KiCad names,
visualizer preferences, or Three.js scene behavior outside this repository.

New browser-capable APIs should normally have:

- a native C++ value API;
- a flat C ABI entry point for WASM and future non-C++ callers;
- byte-buffer inputs when the browser cannot rely on local files;
- documented ownership rules for returned strings, byte buffers, or mesh
  packets;
- options encoded in a stable JSON object when the option surface is expected
  to grow;
- version and ABI notes when the callable surface changes;
- at least one native test and one WASM/browser validation path.

For STEP model rendering, prefer a backend-neutral mesh/tessellation packet if
it can stay compact and practical. Returning GLB bytes is acceptable when it
keeps the first integration simple, but it should not be the only long-term
geometry transport considered for browser tools.
## Header Entry Point

Use the umbrella header for native C++ callers:

```cpp
#include "geometer.h"
```

That includes the current public headers:

- `geometer/status.h`
- `geometer/version.h`
- `geometer/step_to_glb.h`
- `geometer/projection.h`
- `geometer/projection_options_json.h`
- `geometer/planar_contours.h`
- `geometer/planar_solve.h`
- `geometer/planar_step.h`
## Version

Defined in `src/cpp/lib/geometer/version.h`.

Geometer uses date-based release versions per ADR 006. The current package and
runtime version is `2026.9.5`, corresponding to release tag `v2026-09-05`.
The current C ABI generation is `20260905`. Consumers should check both the
project version and ABI generation at runtime.

```cpp
struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int abi = 0;
    const char* string = "";
};

const Version& version();
const char* version_string();
int version_major();
int version_minor();
int version_patch();
int abi_version();
```

C ABI version functions:

```c
const char* geometer_version_string(void);
int geometer_version_major(void);
int geometer_version_minor(void);
int geometer_version_patch(void);
int geometer_abi_version(void);
```

WASM consumers can call the same C ABI exports:

```js
const version = module.ccall("geometer_version_string", "string", [], []);
const abi = module.ccall("geometer_abi_version", "number", [], []);
```

The returned version string is static storage and must not be freed.
## Status

Defined in `src/cpp/lib/geometer/status.h`.

```cpp
struct Status {
    int code = 0;
    std::string message;
    bool ok() const;
};
```

Most library functions return `0` on success. When a `Status*` parameter is
available and a failure occurs, `code` and `message` describe the error.
