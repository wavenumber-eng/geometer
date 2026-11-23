# Native executable bundle support

This internal Rust library shares PCBE's existing eight-file Geometer archive
admission and locked, digest-addressed materialization with other native hosts.
It supports the qualified Windows x64 and macOS arm64 package shapes and preserves
the original executable, attestation and six license files. It never downloads,
starts a process, selects a company configuration or interprets a CAD model.

Hosts supply the expected archive digest and a user-local absolute cache root.
They retain their own runtime override policy and lazy worker startup. A resident
host can retain a successfully admitted `Bundle` to avoid repeated archive
decoding. Its `materialize` operation still verifies and repairs the extracted
closure on each worker resolution. Do not memoize transient failures. Publication trust and original-source release validation
remain with the product's build/package producer.

The implementation was factored from the existing PCBE bundle modules without
changing their archive or recovery policy. See Geometer's developer guide and
PCBE's bundled-Geometer design for the existing compatibility qualification.
