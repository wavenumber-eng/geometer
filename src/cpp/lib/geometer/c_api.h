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

    GeometerStringResult geometer_step_hlr_projection_json(GeometerBuffer step_data,
                                                           const char* options_json);

    void geometer_free_string(char* value);

#ifdef __cplusplus
}
#endif
