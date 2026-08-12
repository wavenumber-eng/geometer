# Generic Operation C ABI

## Status and scope

This is the proposed additive generic C ABI reviewed under ADR-011. It serves
native callers and the full browser/Web Worker WASM build. It does not remove
or change existing per-operation symbols.

The first implementation is `geometry.model_bounds.a0`. A successful browser
pilot accepts model bytes as the named `model` attachment and returns
`geometry.model_bounds.a0` JSON through this ABI.

## Declarations

The reviewed header shape is:

```c
#include <stdint.h>

typedef struct GeometerAttachmentView {
    uint32_t struct_size;
    uint32_t flags;
    const char* name;
    uint32_t name_size;
    const char* media_type;
    uint32_t media_type_size;
    const unsigned char* data;
    uint32_t data_size;
    uint32_t reserved0;
} GeometerAttachmentView;

typedef struct GeometerOperationResult GeometerOperationResult;

enum {
    GEOMETER_OPERATION_ABI_OK = 0,
    GEOMETER_OPERATION_ABI_INVALID_ARGUMENT = 1001,
    GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED = 1002,
    GEOMETER_OPERATION_ABI_NO_MEMORY = 1003,
    GEOMETER_OPERATION_ABI_INTERNAL = 1004
};

int geometer_operation_catalog_json(char** value, char** error);

int geometer_operation_execute(
    const char* operation_id,
    uint32_t operation_id_size,
    const unsigned char* request_json,
    uint32_t request_json_size,
    const GeometerAttachmentView* attachments,
    uint32_t attachment_count,
    GeometerOperationResult** result,
    char** error
);

const unsigned char* geometer_operation_result_json_data(
    const GeometerOperationResult* result
);
uint32_t geometer_operation_result_json_size(
    const GeometerOperationResult* result
);
uint32_t geometer_operation_result_attachment_count(
    const GeometerOperationResult* result
);
const char* geometer_operation_result_attachment_name(
    const GeometerOperationResult* result,
    uint32_t index,
    uint32_t* name_size
);
const char* geometer_operation_result_attachment_media_type(
    const GeometerOperationResult* result,
    uint32_t index,
    uint32_t* media_type_size
);
const unsigned char* geometer_operation_result_attachment_data(
    const GeometerOperationResult* result,
    uint32_t index,
    uint32_t* data_size
);
void geometer_operation_result_free(GeometerOperationResult* result);
```

The final C declaration may add compile-time `sizeof`/offset assertions and a
symbol visibility macro, but must not change the reviewed field order or
semantics without returning ADR-011 to review.

## Input layout and validation

All sizes are byte counts. The generic ABI uses `uint32_t` rather than `size_t` so the same
descriptor fields are unambiguous in native and wasm32 builds. Pointers retain
the target platform's normal pointer width.

`struct_size` must equal the A0 descriptor size compiled for the target. This
rejects callers using a different descriptor generation. `flags` and
`reserved0` must be zero. Names and media types are length-delimited UTF-8 and
need not be NUL terminated. Attachment bytes may contain any value.

The call validates, before operation execution:

- all required output pointers;
- pointer/size consistency for every view;
- exact descriptor size, zero flags, and zero reserved fields;
- UTF-8 operation, attachment name, and media type values;
- catalog operation identity;
- unique attachment names;
- attachment count, individual size, and aggregate size limits;
- required and allowed attachment names/media types from the catalog; and
- strict generated request JSON.

The request JSON is an operation-specific generated envelope. Operation
identity is passed separately and is not duplicated inside the envelope.

## Result and ownership

`geometer_operation_execute` is synchronous. On return code `0`, `result` is
non-null and owns one generated response JSON buffer plus zero or more named
output attachments. The accessor pointers are borrowed and remain valid until
`geometer_operation_result_free`.

The caller must free the handle exactly once. It must not free accessor pointers
individually. Accessors return null/zero for a null handle or out-of-range index
and never transfer ownership.

Contract rejection and geometry-operation failure are normal typed operation
outcomes when a response handle can be constructed; they use generated response
diagnostics and still return `0` from the local C call. A nonzero C return means
the ABI invocation itself could not produce a response, for example invalid
pointers, descriptor corruption, or allocation failure. In that case `result`
is null and `error`, when supplied, is an owned string released by the existing
`geometer_free_string`.

The named `GEOMETER_OPERATION_ABI_*` constants are stable only as local ABI
outcomes. Generated clients may classify them as invocation, limit, memory, or
internal transport failures. They are not geometry-operation diagnostics and
must not appear as wire diagnostic codes.

`geometer_operation_catalog_json` returns an owned UTF-8 catalog/capability
document through `value`; the caller uses `geometer_free_string`. It includes
release version, date-based C ABI generation, supported operation identities,
attachment declarations, descriptor layout metadata, and enforced limits.

## C ABI versioning

These names follow the existing unadorned C ABI style. Compatibility is
governed by ADR-006's date-based C ABI generation and reported by
`geometer_abi_version()`. Schema-style `a0` suffixes are reserved for contract
identities and executable IPC A0; they are not a second C ABI version scheme.

`struct_size`, an opaque result type, accessors, and catalog-reported descriptor
layout allow compatible evolution without changing the function signatures.
If an incompatible future C ABI cannot be expressed through those mechanisms,
it requires a new ABI decision, a date-generation change, and explicitly new
entry-point names rather than pre-emptively suffixing every current symbol.

## Hard limits

Implementations may advertise smaller runtime limits, but never larger values
for this C ABI generation:

| Item | A0 maximum |
| --- | ---: |
| Operation identity | 128 bytes |
| Request or response JSON | 8 MiB |
| Attachment count | 16 |
| Attachment name | 128 bytes |
| Media type | 128 bytes |
| Individual attachment | 256 MiB |
| Aggregate input or output attachments | 512 MiB native, 256 MiB wasm32 |

Lengths and sums are validated with overflow-safe arithmetic before copying or
allocating. Zero-length data may use a null pointer; nonzero data may not.

## Threading

The ABI adds no concurrent-execution guarantee. The first implementation may
serialize calls internally. A caller must not infer that OCCT operations can
overlap merely because separate result handles exist.

## Emscripten requirements

The full browser target exports every declaration above plus existing
allocation/free functions. The generated TypeScript adapter owns descriptor
layout, allocation, copying, accessor calls, output copying, and cleanup in a
`try/finally` path. User-facing demos and clients do not marshal these pointers
directly.

The normalized operation catalog generates or mechanically verifies the header
declarations and Emscripten export list. Build validation fails when they drift.
