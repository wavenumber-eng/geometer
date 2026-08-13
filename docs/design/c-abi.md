# C ABI

The retained per-operation ABI and the additive generic operation ABI are both
implemented. The generic ABI is the generated-client path for newly promoted
operations; the existing symbols remain available for compatibility. Its
normative ownership, layout, limit, and failure rules are specified in
[Generic Operation C ABI](generic-operation-c-abi.md).

Defined in `src/cpp/lib/geometer/c_api.h`.

```c
typedef struct GeometerBuffer {
    const unsigned char* data;
    size_t size;
} GeometerBuffer;

typedef struct GeometerStringResult {
    int code;
    char* value;
    char* error;
} GeometerStringResult;

typedef struct GeometerByteResult {
    int code;
    unsigned char* value;
    size_t size;
    char* error;
} GeometerByteResult;

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
uint32_t geometer_operation_result_json_size(const GeometerOperationResult* result);
uint32_t geometer_operation_result_attachment_count(
    const GeometerOperationResult* result
);
const char* geometer_operation_result_attachment_name(
    const GeometerOperationResult* result,
    uint32_t index,
    uint32_t* size
);
const char* geometer_operation_result_attachment_media_type(
    const GeometerOperationResult* result,
    uint32_t index,
    uint32_t* size
);
const unsigned char* geometer_operation_result_attachment_data(
    const GeometerOperationResult* result,
    uint32_t index,
    uint32_t* size
);
void geometer_operation_result_free(GeometerOperationResult* result);

GeometerStringResult geometer_step_hlr_projection_json(
    GeometerBuffer step_data,
    const char* options_json
);

int geometer_step_hlr_projection_json_bytes(
    const unsigned char* step_data,
    size_t step_size,
    const char* options_json,
    char** value,
    char** error
);

GeometerByteResult geometer_step_to_glb(
    GeometerBuffer step_data,
    const char* options_json
);

int geometer_step_to_glb_bytes(
    const unsigned char* step_data,
    size_t step_size,
    const char* options_json,
    unsigned char** value,
    size_t* value_size,
    char** error
);

GeometerByteResult geometer_planar_batch_solve(GeometerBuffer request_data);

GeometerStringResult geometer_planar_batch_solve_json(GeometerBuffer request_data);

int geometer_planar_batch_solve_bytes(
    const unsigned char* request_data,
    size_t request_size,
    unsigned char** value,
    size_t* value_size,
    char** error
);

int geometer_planar_batch_solve_json_bytes(
    const unsigned char* request_data,
    size_t request_size,
    char** value,
    char** error
);

GeometerByteResult geometer_planar_triangulate(GeometerBuffer request_data);

int geometer_planar_triangulate_bytes(
    const unsigned char* request_data,
    size_t request_size,
    unsigned char** value,
    size_t* value_size,
    char** error
);

GeometerByteResult geometer_clipper2_boolean(GeometerBuffer request_data);

int geometer_clipper2_boolean_bytes(
    const unsigned char* request_data,
    size_t request_size,
    unsigned char** value,
    size_t* value_size,
    char** error
);

GeometerByteResult geometer_clipper2_inflate_open(GeometerBuffer request_data);

int geometer_clipper2_inflate_open_bytes(
    const unsigned char* request_data,
    size_t request_size,
    unsigned char** value,
    size_t* value_size,
    char** error
);

const char* geometer_version_string(void);
int geometer_version_major(void);
int geometer_version_minor(void);
int geometer_version_patch(void);
int geometer_abi_version(void);

void geometer_free_string(char* value);
void geometer_free_bytes(unsigned char* value);
```

Returned `error` strings, projection JSON strings, and planar solve JSON strings
are heap-allocated and owned by the caller. Release them with
`geometer_free_string`. Returned GLB and planar byte buffers are heap-allocated
and owned by the caller. Release them with `geometer_free_bytes`. The version
string is static storage and does not use either free function.

The generic result owns generated `geometer.operation.outcome.a0` JSON and any
output attachments. Accessor pointers are borrowed until
`geometer_operation_result_free`. The initial operation catalog contains
`geometry.model_bounds.a0`; it accepts strict generated options JSON and a
required `model` attachment with media type `application/step` or `model/step`.
The catalog reports the release/C ABI generations, descriptor layouts, limits,
and operation attachment declarations so clients do not hard-code them.
