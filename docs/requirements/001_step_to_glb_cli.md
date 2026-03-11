# REQ-001: STEP to GLB CLI conversion

## Summary

Provide a command-line interface to convert STEP files to binary glTF (GLB) format.

## Requirements

1. Accept a STEP (.step, .stp) input file path and a GLB output file path.
2. Preserve colors and names from the STEP model in the GLB output.
3. Support configurable tessellation via linear deflection and angular deflection parameters.
4. Produce a statically linked binary with no runtime dependencies.
5. Build and run on Windows, macOS, and Linux.
6. Return exit code 0 on success, non-zero on failure with a diagnostic on stderr.
