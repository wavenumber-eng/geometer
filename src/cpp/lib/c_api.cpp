#include "geometer/c_api.h"

#include "geometer/projection.h"
#include "geometer/projection_options_json.h"
#include "geometer/version.h"

#include <cstdlib>
#include <cstring>
#include <string>

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
