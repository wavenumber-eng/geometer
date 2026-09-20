+++
type = "plan_log"
id = "local-builder-qualification-2026-09-20"
plan_id = "release-cycle-time-and-artifact-pipeline"
step_id = "local-builder"
created = "2026-09-20T14:20:00-04:00"
+++

# Log: local builder qualification

## Dedicated WSL2 builder

A dedicated `Geometer-Ubuntu-22.04` WSL2 distribution now lives on the data
volume. Its build user has no sudo membership. `/etc/wsl.conf` disables Windows
drive automounting, Windows executable interoperability, and inherited Windows
`PATH` entries. After restart, checks proved that `/mnt/c/Windows` and
`cmd.exe` were unavailable.

The checked-in provisioning recipe installs the Ubuntu 22.04 native build
dependencies and pins downloaded tools by version and SHA-256:

| Tool | Version |
| --- | --- |
| uv | 0.9.15 |
| CMake | 4.1.2 |
| Chrome for Testing | 153.0.8010.52 |
| Node | 24.12.0 |
| npm | 11.16.0 |
| Rust | 1.95.0 with `clippy` and `rustfmt` |

The qualification checkout was clean, contained no `.env`, and resolved to
`98a72fcd45acc3f3ac96becc971ec4d3e8e25f3e`. It ran from the WSL ext4
filesystem, not `/mnt/c`.

## Linux x64 observations

| Work | Wall time | Result |
| --- | ---: | --- |
| Locked OCCT restore from R2 | 6.7 sec | exact `linux-x64-gcc11-static` archive restored |
| Repeated OCCT lock check | 0.07 sec | local install reused, no network/build |
| Clean native configure/build/smoke/20 CTests | about 88 sec | pass; promotion-attested clean source |
| Warm native validation | 7.4 sec | pass |
| Wheel build and installed-package qualification | 11.4 sec | pass |
| Clean separate SDK graph compile | about 32 sec | build passed; packaging initially stopped on the expected dirty-`dist` provenance guard |
| Warm SDK packaging after exact `dist` restore | 6.9 sec | pass |
| Relocated C/Rust SDK qualification | about 68 sec | pass |

The SDK validation currently compiles the Rust dependency graph once for the
direct-static test and again for the packaged illustration example. That is
measured duplicate work for the build-graph/test audit; it is not a reason to
add another cache identity.

The first native attempt failed before compilation because Ubuntu 22.04's apt
CMake is 3.22 while Geometer requires 3.24 or newer. The provisioning recipe
now installs checksum-pinned CMake 4.1.2. The first WASM contract check also
proved that Rust's minimal profile omits required `rustfmt`; the recipe now
installs pinned `rustfmt` and `clippy` components.

## WASM observations

| Work | Wall time | Result |
| --- | ---: | --- |
| One-time emsdk download/install | about 60 sec | Emscripten 3.1.56 installed |
| Clean Geometer WASM compile/link with restored OCCT and emsdk | about 90 sec | pass |
| Warm WASM build | 12.3 sec | pass; Ninja had no work |
| Release-equivalent site builds and Node/static validation | 14.8 sec | pass |
| Full Chrome-backed Rack WASM stratum | 37.4 sec | 12 passed, 6/6 subtests |

The first full WASM stratum run found three native/WASM catalog mismatches only
because the local manual sequence had restored the repository's older tracked
Linux executable to obtain clean SDK provenance. Copying the already-qualified
native executable back to `dist` fixed all three without compilation. The
shared candidate command must encode SDK-before-native ordering and make this
state transition explicit.

The pinned Chrome addition changed the two browser-site tests from skips to
passes. The exact Node validations also passed for model bounds, planar bytes,
and STEP-to-GLB.

## Remaining before this step is complete

- Run the clean isolated Windows x64 native, wheel, static-SDK, and relocated
  SDK sequence end to end and record clean/warm timings.
- Turn the proven commands into one shared candidate command before registering
  a manual-only one-job runner.
- Run three clean and three warm measurements for the final command; the
  measurements above are implementation qualification, not the final p50/worst
  acceptance set.

## Windows lock correction discovered during qualification

Restoring a Windows OCCT archive into the `D:` worktree initially failed
because the verified archive was staged in the user's `C:` temporary directory
and `Path.replace` cannot rename across Windows volumes. Locked restores now
stage under the target install parent. A focused regression test requires that
same-volume rule; no copy fallback or additional identity was introduced.

The `/MT` archive then failed its link probe under local MSVC 19.44 with missing
`__std_max_element_d_` and `__std_min_element_4i` symbols. Producer run
`35477956256` proves that archive was built by Visual Studio 18 / MSVC 19.51
(`VCToolsVersion=14.51.36231`) even though its internal profile claims
`msvc-v143-crt-static`. The old producer collapsed every MSVC version newer
than 14.30 to `v143`, so the archive's ABI evidence is false.

The producer now identifies 14.50+ as `msvc-v145`, which makes the current lock
reject that toolchain before OCCT configure/build. Windows producer, release,
and transport-baseline workflows are pinned to `windows-2022`, matching the
governed `msvc-v143` lock and this workstation's Visual Studio 2022 toolchain.
The mislabeled `/MT` object remains immutable historical input; it must be
replaced by a newly built Windows-2022 candidate and the lock must move to that
new digest before Windows SDK qualification can complete.
