+++
type = "adr"
id = "geometer-adr-006"
domain = "geometer"
status = "accepted"
title = "Use Date-Based Versioning"
created = "2026-08-18"
+++

# ADR 006: Date-Based Versioning Policy

## Status

Accepted

## Context

Geometer uses PyPI releases and downstream consumption by internal Monkey
tools. Those tools use date-oriented release names, and the release date is more
meaningful for this workflow than semantic major/minor/patch numbers.

Different consumers need different version formats:

- humans prefer zero-padded calendar dates;
- PyPI requires PEP 440-compatible normalized versions and removes leading
  zeros from release segments;
- CMake `project(VERSION ...)` accepts numeric dotted versions, not arbitrary
  development strings;
- C and C++ callers sometimes need cheap numeric comparisons for ABI/runtime
  checks.

Optional build timestamps or build numbers may become useful later, but they
are not needed to define the release policy now.

## Decision

Geometer releases use a UTC date as the release identity.

Human-facing Git tags, release names, and planning milestones use the
zero-padded form:

```text
vYYYY-MM-DD
```

Same-day follow-up releases append a monotonically increasing serial:

```text
vYYYY-MM-DD-N
```

Packaging and build-tool versions use the dotted PEP 440/CMake-compatible form:

```text
YYYY.M.D
```

Same-day follow-up releases use:

```text
YYYY.M.D.N
```

For example, the release tag `v2026-05-23` maps to package/build version
`2026.5.23`.

For example, the follow-up release tag `v2026-05-24-2` maps to package/build
version `2026.5.24.2`.

C ABI generation uses an integer date:

```text
YYYYMMDD
```

The ABI date changes when Geometer publishes a new C ABI generation. It is not a
per-build timestamp and it does not include same-day release serials. Python
and downstream tools should check both the package version and C ABI generation
when they depend on a specific interface.

All generated date/build metadata must use UTC. Local time zones must not affect
release artifacts.

The release version is the only required public package version. Development
build numbers, seconds-since-midnight, Unix epoch seconds, CI run numbers, and
git SHA fields are optional diagnostics that can be added later. If added, they
should not be required for normal public package resolution.

## Implementation Direction

Add one version source of truth that can generate:

- the human release tag/date;
- the CMake project version;
- `version_config.h`;
- the Python package version;
- the C ABI generation integer.

Until that generator exists, update the root CMake version, Python package
version, and C ABI version manually when preparing a release.

## Consequences

The human release date keeps leading zeros where they improve readability.
Package manager versions intentionally do not.

The policy avoids maintaining separate semantic-version and date-version
systems.

Build metadata can be introduced later without changing the public release
scheme.

Downstream consumers should treat dated Geometer releases like other Monkey
tool releases and plan migrations against a specific release date.
