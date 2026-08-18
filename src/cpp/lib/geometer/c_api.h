#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(GEOMETER_BUILD_SHARED)
#define GEOMETER_C_API __declspec(dllexport)
#else
#define GEOMETER_C_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct GeometerBuffer
    {
        const unsigned char* data;
        size_t size;
    } GeometerBuffer;

    typedef struct GeometerStringResult
    {
        int code;
        char* value;
        char* error;
    } GeometerStringResult;

    typedef struct GeometerByteResult
    {
        int code;
        unsigned char* value;
        size_t size;
        char* error;
    } GeometerByteResult;

    typedef struct GeometerAttachmentView
    {
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

    enum
    {
        GEOMETER_OPERATION_ABI_OK = 0,
        GEOMETER_OPERATION_ABI_INVALID_ARGUMENT = 1001,
        GEOMETER_OPERATION_ABI_LIMIT_EXCEEDED = 1002,
        GEOMETER_OPERATION_ABI_NO_MEMORY = 1003,
        GEOMETER_OPERATION_ABI_INTERNAL = 1004,
    };

    GEOMETER_C_API int geometer_operation_catalog_json(char** value, char** error);

    GEOMETER_C_API int
    geometer_operation_execute(const char* operation_id, uint32_t operation_id_size,
                               const unsigned char* request_json, uint32_t request_json_size,
                               const GeometerAttachmentView* attachments, uint32_t attachment_count,
                               GeometerOperationResult** result, char** error);

    GEOMETER_C_API const unsigned char*
    geometer_operation_result_json_data(const GeometerOperationResult* result);
    GEOMETER_C_API uint32_t
    geometer_operation_result_json_size(const GeometerOperationResult* result);
    GEOMETER_C_API uint32_t
    geometer_operation_result_attachment_count(const GeometerOperationResult* result);
    GEOMETER_C_API const char*
    geometer_operation_result_attachment_name(const GeometerOperationResult* result, uint32_t index,
                                              uint32_t* size);
    GEOMETER_C_API const char*
    geometer_operation_result_attachment_media_type(const GeometerOperationResult* result,
                                                    uint32_t index, uint32_t* size);
    GEOMETER_C_API const unsigned char*
    geometer_operation_result_attachment_data(const GeometerOperationResult* result, uint32_t index,
                                              uint32_t* size);
    GEOMETER_C_API void geometer_operation_result_free(GeometerOperationResult* result);

    GEOMETER_C_API GeometerStringResult geometer_step_hlr_projection_json(GeometerBuffer step_data,
                                                                          const char* options_json);

    GEOMETER_C_API int geometer_step_hlr_projection_json_bytes(const unsigned char* step_data,
                                                               size_t step_size,
                                                               const char* options_json,
                                                               char** value, char** error);

    GEOMETER_C_API GeometerByteResult geometer_step_to_glb(GeometerBuffer step_data,
                                                           const char* options_json);

    GEOMETER_C_API int geometer_step_to_glb_bytes(const unsigned char* step_data, size_t step_size,
                                                  const char* options_json, unsigned char** value,
                                                  size_t* value_size, char** error);

    GEOMETER_C_API GeometerByteResult geometer_planar_batch_solve(GeometerBuffer request_data);

    GEOMETER_C_API int geometer_planar_batch_solve_bytes(const unsigned char* request_data,
                                                         size_t request_size, unsigned char** value,
                                                         size_t* value_size, char** error);

    GEOMETER_C_API GeometerStringResult
    geometer_planar_batch_solve_json(GeometerBuffer request_data);

    GEOMETER_C_API int geometer_planar_batch_solve_json_bytes(const unsigned char* request_data,
                                                              size_t request_size, char** value,
                                                              char** error);

    GEOMETER_C_API GeometerByteResult geometer_planar_triangulate(GeometerBuffer request_data);

    GEOMETER_C_API int geometer_planar_triangulate_bytes(const unsigned char* request_data,
                                                         size_t request_size, unsigned char** value,
                                                         size_t* value_size, char** error);

    GEOMETER_C_API GeometerByteResult geometer_clipper2_boolean(GeometerBuffer request_data);

    GEOMETER_C_API int geometer_clipper2_boolean_bytes(const unsigned char* request_data,
                                                       size_t request_size, unsigned char** value,
                                                       size_t* value_size, char** error);

    GEOMETER_C_API GeometerByteResult geometer_clipper2_inflate_open(GeometerBuffer request_data);

    GEOMETER_C_API int geometer_clipper2_inflate_open_bytes(const unsigned char* request_data,
                                                            size_t request_size,
                                                            unsigned char** value,
                                                            size_t* value_size, char** error);

    GEOMETER_C_API const char* geometer_version_string(void);
    GEOMETER_C_API int geometer_version_major(void);
    GEOMETER_C_API int geometer_version_minor(void);
    GEOMETER_C_API int geometer_version_patch(void);
    GEOMETER_C_API int geometer_abi_version(void);

    GEOMETER_C_API void geometer_free_string(char* value);
    GEOMETER_C_API void geometer_free_bytes(unsigned char* value);

#ifdef __cplusplus
}

static_assert(offsetof(GeometerAttachmentView, struct_size) == 0);
static_assert(offsetof(GeometerAttachmentView, flags) == 4);
#if INTPTR_MAX == INT64_MAX
static_assert(offsetof(GeometerAttachmentView, name) == 8);
static_assert(offsetof(GeometerAttachmentView, name_size) == 16);
static_assert(offsetof(GeometerAttachmentView, media_type) == 24);
static_assert(offsetof(GeometerAttachmentView, media_type_size) == 32);
static_assert(offsetof(GeometerAttachmentView, data) == 40);
static_assert(offsetof(GeometerAttachmentView, data_size) == 48);
static_assert(offsetof(GeometerAttachmentView, reserved0) == 52);
static_assert(sizeof(GeometerAttachmentView) == 56);
#else
static_assert(offsetof(GeometerAttachmentView, name) == 8);
static_assert(offsetof(GeometerAttachmentView, name_size) == 12);
static_assert(offsetof(GeometerAttachmentView, media_type) == 16);
static_assert(offsetof(GeometerAttachmentView, media_type_size) == 20);
static_assert(offsetof(GeometerAttachmentView, data) == 24);
static_assert(offsetof(GeometerAttachmentView, data_size) == 28);
static_assert(offsetof(GeometerAttachmentView, reserved0) == 32);
static_assert(sizeof(GeometerAttachmentView) == 36);
#endif
#endif

#undef GEOMETER_C_API
