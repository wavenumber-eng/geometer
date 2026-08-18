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

The final C declaration adds the existing symbol visibility macro and
compile-time `sizeof`/offset assertions. It must not change the reviewed field
order or semantics without returning ADR-011 to review.

## Input layout and validation

All sizes are byte counts. The generic ABI uses `uint32_t` rather than `size_t`
so the same descriptor fields are unambiguous in native and wasm32 builds.
Pointers retain the target platform's normal pointer width. Supported builds
must satisfy these asserted layouts:

| Field | wasm32 offset | 64-bit offset |
| --- | ---: | ---: |
| `struct_size` | 0 | 0 |
| `flags` | 4 | 4 |
| `name` | 8 | 8 |
| `name_size` | 12 | 16 |
| `media_type` | 16 | 24 |
| `media_type_size` | 20 | 32 |
| `data` | 24 | 40 |
| `data_size` | 28 | 48 |
| `reserved0` | 32 | 52 |
| total `sizeof(GeometerAttachmentView)` | 36 | 56 |

A build whose compiler ABI does not produce the applicable layout fails at
compile time. The catalog reports pointer width, total size, and every field
offset so a generated adapter verifies its view before its first call.

`struct_size` must equal the descriptor size compiled for the target. This
rejects callers using a different descriptor layout. It does not authorize a
caller to append fields. `flags` and `reserved0` must be zero. Names and media
types are length-delimited UTF-8 and need not be NUL terminated. Catalog
operation identities and attachment names are nonempty ASCII tokens without
embedded NUL; media types are nonempty catalog-declared ASCII values.
Attachment bytes may contain any value.

The call validates, before operation execution:

- non-null required output holders and non-aliasing output-holder addresses;
- pointer/size consistency for every view;
- exact descriptor size, zero flags, and zero reserved fields;
- UTF-8 operation, attachment name, and media type values;
- catalog operation identity;
- unique attachment names;
- attachment count, individual size, and aggregate size limits;
- required and allowed attachment names/media types from the catalog; and
- strict generated request JSON.

The caller supplies valid readable input ranges and writable output holders;
the ABI can validate lengths and null consistency but cannot prove arbitrary
foreign pointers are accessible. All input pointers are borrowed only for the
duration of the synchronous call. The result never adopts or retains them.

The request JSON is the generated operation-specific request DTO, not a second
transport envelope. Operation identity is passed separately and is not
duplicated in that JSON. On a local invocation that was structurally valid, the
result JSON is the generated generic operation outcome: it contains the
operation identity, `ok`, an operation-specific result value when successful,
and governed diagnostics when unsuccessful. This outcome shape is shared with
IPC responses. IPC requests add their own generated frame JSON around the same
operation-specific request DTO because IPC has no separate function argument.

## Result and ownership

`geometer_operation_execute` is synchronous. `result` is required; `error` is
optional. The function initializes `*result` to null and, when `error` is
non-null, initializes `*error` to null before validation that can fail. Its
output states are:

| Return | `*result` | `*error` when supplied | Meaning |
| ---: | --- | --- | --- |
| `0` | non-null | null | A typed operation outcome was produced. |
| nonzero | null | null or owned NUL-terminated string | Local ABI invocation failed. |

On return code `0`, the result owns one nonempty generated response JSON buffer
plus zero or more named output attachments. The accessor pointers are borrowed
and remain valid until `geometer_operation_result_free`.

The caller must free the handle exactly once. It must not free accessor pointers
individually. `geometer_operation_result_free(NULL)` is a no-op. The name,
media-type, and data accessors require their size output pointer; they set it to
zero before other validation. They return null for a null handle, null size
pointer, or out-of-range index and never transfer ownership. A declared
zero-length attachment may validly return null data with size zero; attachment
names and media types are never empty. The JSON data accessor returns null only
for a null handle. JSON size and attachment count return zero for a null handle.

Contract rejection and geometry-operation failure are normal typed operation
outcomes when a response handle can be constructed; they use generated response
diagnostics and still return `0` from the local C call. A nonzero C return means
the ABI invocation itself could not produce a response, for example invalid
pointers, descriptor corruption, or allocation failure. In that case `result`
is null and `error`, when supplied, is an owned string released by the existing
`geometer_free_string`.

Local return classification is exact:

| Return | Conditions |
| --- | --- |
| `GEOMETER_OPERATION_ABI_INVALID_ARGUMENT` | Missing/aliasing output holder, pointer/size inconsistency, wrong `struct_size`, nonzero flags/reserved fields, or malformed operation-identity bytes. |
| `GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED` | A raw count/length/sum exceeds the C ABI hard maximum or overflows before a typed request can be constructed. |
| `GEOMETER_OPERATION_ABI_NO_MEMORY` | Required response, diagnostic, or error storage cannot be allocated. |
| `GEOMETER_OPERATION_ABI_INTERNAL` | An unexpected implementation failure escaped normal operation diagnostics. |

A well-formed but unknown operation identity, invalid request JSON, duplicate or
undeclared attachment name, media-type mismatch, or operation-specific limit
failure produces a typed outcome and local return `0`. This boundary keeps
foreign-memory and descriptor failures separate from governed contract and
operation diagnostics.

The named `GEOMETER_OPERATION_ABI_*` constants are stable only as local ABI
outcomes. Generated clients may classify them as invocation, limit, memory, or
internal transport failures. They are not geometry-operation diagnostics and
must not appear as wire diagnostic codes.

`geometer_operation_catalog_json` requires `value` and accepts optional
`error`; their holder addresses must not alias. When `value` is non-null, it is
initialized to null, and when `error` is non-null, it is initialized to null,
before later validation. On success it returns an owned, NUL-terminated UTF-8
catalog/capability document through `value`; the caller uses
`geometer_free_string`. It includes release version, date-based C ABI
generation, supported operation identities, attachment declarations,
descriptor layout metadata, and enforced limits.

No C++ exception may cross any C ABI boundary. Every entry point catches all
exceptions and maps them to a local ABI failure with outputs in the states
above. Error-string allocation failure is allowed to leave `*error` null.

## C ABI versioning

These names follow the existing unadorned C ABI style. Compatibility is
governed by ADR-006's date-based C ABI generation and reported by
`geometer_abi_version()`. Schema-style `a0` suffixes are reserved for contract
identities and executable IPC A0; they are not a second C ABI version scheme.

The opaque result type, additive accessors, currently reserved fields, and
catalog metadata allow some compatible evolution without changing these
function signatures. `struct_size` detects layout mismatch; because this
generation requires an exact size, it is not a trailing-struct-extension
mechanism. A future descriptor layout or other incompatible C ABI requires a
new ABI decision, a date-generation change, and explicitly new entry-point
names rather than pre-emptively suffixing every current symbol.

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

The ABI adds no concurrent-execution or reentrancy guarantee. Callers and
generated adapters must serialize catalog and execute calls. Result access and
freeing must not race with another access or free of the same handle. A caller
must not infer that OCCT operations can overlap merely because separate result
handles exist. A later concurrency guarantee requires a reviewed ABI policy.

## Emscripten requirements

The full browser target exports every declaration above plus existing
allocation/free functions. The generated TypeScript adapter owns descriptor
layout, allocation, copying, accessor calls, output copying, and cleanup in a
`try/finally` path. User-facing demos and clients do not marshal these pointers
directly.

The normalized operation catalog generates or mechanically verifies the header
declarations and Emscripten export list. Build validation fails when they drift.
