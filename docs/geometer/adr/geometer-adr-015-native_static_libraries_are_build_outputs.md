+++
type = "adr"
id = "geometer-adr-015"
domain = "geometer"
status = "accepted"
title = "Native Static Libraries Are Build Outputs"
created = "2026-08-30"
+++

# ADR-015: Native static libraries are build outputs

## Status

Accepted.

## Context

Geometer historically copied its native static library into
`dist/native/<platform>/` and committed it alongside runtime artifacts. On
Windows the build also copied the same bytes under both `geometer.lib` and
`libgeometer.a`. The archive grew beyond GitHub's recommended per-file size.

No known Wavenumber consumer links either persisted filename. Current consumers
use the native executable through the Python package or executable IPC, or use
the browser WASM distribution. The native release archive also lacks the public
header installation, exported CMake targets, toolchain/ABI contract, and OCCT
dependency-link closure required for a usable native SDK.

## Decision

Native `.lib` and `.a` files are generated build/cache outputs:

- CMake continues to build `geometer_lib` for Geometer targets and tests;
- the library remains under the configured CMake build tree;
- CMake does not copy it into `dist/native/<platform>/`;
- Git ignores native libraries under `dist/native/`;
- native runtime archives exclude and reject `.lib` and `.a` files; and
- CI caches may retain build-tree libraries only as an optimization, never as
  an authoritative distribution channel.

The native executable, its build attestation, licenses, Python wheels, and WASM
artifacts retain their existing distribution channels.

## Future native SDK

A native SDK may be introduced only as a separately named, versioned artifact
with:

- the supported public headers;
- exported CMake package targets;
- compiler, runtime, architecture, and ABI compatibility metadata;
- required notices and licenses;
- an explicit OCCT and other transitive dependency strategy; and
- an external-consumer build and link test.

A bare static archive is not considered a supported SDK.

## Consequences

New commits and runtime releases no longer grow by one static-library blob per
platform build. Existing blobs remain in reachable Git history until a
separately approved repository-history rewrite. Full historical clones may
therefore still download those objects; shallow or partial clones may avoid
some historical payload.
