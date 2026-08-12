# C ABI

The current implemented per-operation ABI is documented below. The additive
generic operation ABI is specified in
[Generic Operation C ABI](generic-operation-c-abi.md) and remains unimplemented
until ADR-011 completes independent design review.

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
