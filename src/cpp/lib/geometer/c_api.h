#pragma once

#include <stddef.h>

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
#endif

#undef GEOMETER_C_API
