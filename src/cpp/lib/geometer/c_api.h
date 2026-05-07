#pragma once

#include <stddef.h>

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

    GeometerStringResult geometer_step_hlr_projection_json(GeometerBuffer step_data,
                                                           const char* options_json);

    int geometer_step_hlr_projection_json_bytes(const unsigned char* step_data, size_t step_size,
                                                const char* options_json, char** value,
                                                char** error);

    GeometerByteResult geometer_step_to_glb(GeometerBuffer step_data, const char* options_json);

    int geometer_step_to_glb_bytes(const unsigned char* step_data, size_t step_size,
                                   const char* options_json, unsigned char** value,
                                   size_t* value_size, char** error);

    GeometerByteResult geometer_planar_batch_solve(GeometerBuffer request_data);

    int geometer_planar_batch_solve_bytes(const unsigned char* request_data, size_t request_size,
                                          unsigned char** value, size_t* value_size,
                                          char** error);

    const char* geometer_version_string(void);
    int geometer_version_major(void);
    int geometer_version_minor(void);
    int geometer_version_patch(void);
    int geometer_abi_version(void);

    void geometer_free_string(char* value);
    void geometer_free_bytes(unsigned char* value);

#ifdef __cplusplus
}
#endif
