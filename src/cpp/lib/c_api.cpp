#include "geometer/c_api.h"

#include "geometer/planar_solve.h"
#include "geometer/projection.h"
#include "geometer/projection_options_json.h"
#include "geometer/step_to_glb.h"
#include "geometer/step_to_glb_options_json.h"
#include "geometer/version.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{

char* copy_c_string(const char* text)
{
    const char* source = text == nullptr ? "" : text;
    const std::size_t length = std::strlen(source);
    char* result = static_cast<char*>(std::malloc(length + 1));
    if (result == nullptr)
    {
        return nullptr;
    }
    std::memcpy(result, source, length);
    result[length] = '\0';
    return result;
}

int assign_error(int code, const std::string& message, char** error)
{
    char* text = copy_c_string(message.c_str());
    if (text == nullptr)
    {
        return 94;
    }
    *error = text;
    return code;
}

int copy_byte_vector(const std::vector<unsigned char>& source, unsigned char** value,
                     std::size_t* value_size, const char* label, char** error)
{
    unsigned char* bytes = static_cast<unsigned char*>(std::malloc(source.size()));
    if (bytes == nullptr)
    {
        return assign_error(94, std::string("Failed allocating ") + label + " byte result.", error);
    }
    if (!source.empty())
    {
        std::memcpy(bytes, source.data(), source.size());
    }
    *value = bytes;
    *value_size = source.size();
    return 0;
}

} // namespace

GeometerStringResult geometer_step_hlr_projection_json(GeometerBuffer step_data,
                                                       const char* options_json)
{
    GeometerStringResult result;
    result.value = nullptr;
    result.error = nullptr;
    result.code = geometer_step_hlr_projection_json_bytes(
        step_data.data, step_data.size, options_json, &result.value, &result.error);
    return result;
}

int geometer_step_hlr_projection_json_bytes(const unsigned char* step_data, std::size_t step_size,
                                            const char* options_json, char** value, char** error)
{
    if (value == nullptr || error == nullptr)
    {
        return 93;
    }
    *value = nullptr;
    *error = nullptr;

    geometer::HlrProjectionOptions projection_options;
    geometer::Status status;
    int code =
        geometer::parse_hlr_projection_options_json(options_json, &projection_options, &status);
    if (code != 0)
    {
        return assign_error(code, status.message, error);
    }

    geometer::HlrProjectionResult projection_result;
    code = geometer::step_hlr_projection_from_bytes(step_data, step_size, projection_options,
                                                    &projection_result, &status);
    if (code != 0)
    {
        return assign_error(code, status.message, error);
    }

    std::string json;
    code = geometer::write_hlr_projection_json(projection_result, &json, &status);
    if (code != 0)
    {
        return assign_error(code, status.message, error);
    }

    *value = copy_c_string(json.c_str());
    if (*value == nullptr)
    {
        return assign_error(94, "Failed allocating projection JSON result.", error);
    }
    return 0;
}

GeometerByteResult geometer_step_to_glb(GeometerBuffer step_data, const char* options_json)
{
    GeometerByteResult result;
    result.value = nullptr;
    result.size = 0;
    result.error = nullptr;
    result.code = geometer_step_to_glb_bytes(step_data.data, step_data.size, options_json,
                                             &result.value, &result.size, &result.error);
    return result;
}

int geometer_step_to_glb_bytes(const unsigned char* step_data, std::size_t step_size,
                               const char* options_json, unsigned char** value,
                               std::size_t* value_size, char** error)
{
    if (value == nullptr || value_size == nullptr || error == nullptr)
    {
        return 93;
    }
    *value = nullptr;
    *value_size = 0;
    *error = nullptr;

    geometer::StepToGlbOptions options;
    geometer::Status status;
    int code = geometer::parse_step_to_glb_options_json(options_json, &options, &status);
    if (code != 0)
    {
        return assign_error(code, status.message, error);
    }

    std::vector<unsigned char> glb_bytes;
    code = geometer::step_to_glb_from_bytes(step_data, step_size, options, &glb_bytes, &status);
    if (code != 0)
    {
        return assign_error(code, status.message, error);
    }
    if (glb_bytes.empty())
    {
        return assign_error(3, "STEP-to-GLB conversion returned empty GLB bytes.", error);
    }

    return copy_byte_vector(glb_bytes, value, value_size, "GLB", error);
}

GeometerByteResult geometer_planar_batch_solve(GeometerBuffer request_data)
{
    GeometerByteResult result;
    result.value = nullptr;
    result.size = 0;
    result.error = nullptr;
    result.code = geometer_planar_batch_solve_bytes(
        request_data.data, request_data.size, &result.value, &result.size, &result.error);
    return result;
}

int geometer_planar_batch_solve_bytes(const unsigned char* request_data, std::size_t request_size,
                                      unsigned char** value, std::size_t* value_size, char** error)
{
    if (value == nullptr || value_size == nullptr || error == nullptr)
    {
        return 93;
    }
    *value = nullptr;
    *value_size = 0;
    *error = nullptr;

    geometer::Status status;
    std::vector<unsigned char> response;
    const int code =
        geometer::solve_planar_batch_from_bytes(request_data, request_size, &response, &status);
    if (code != 0)
    {
        return assign_error(code, status.message, error);
    }
    if (response.empty())
    {
        return assign_error(3, "Planar batch solve returned empty bytes.", error);
    }

    return copy_byte_vector(response, value, value_size, "planar batch solve", error);
}

const char* geometer_version_string(void)
{
    return geometer::version_string();
}

int geometer_version_major(void)
{
    return geometer::version_major();
}

int geometer_version_minor(void)
{
    return geometer::version_minor();
}

int geometer_version_patch(void)
{
    return geometer::version_patch();
}

int geometer_abi_version(void)
{
    return geometer::abi_version();
}

void geometer_free_string(char* value)
{
    std::free(value);
}

void geometer_free_bytes(unsigned char* value)
{
    std::free(value);
}
