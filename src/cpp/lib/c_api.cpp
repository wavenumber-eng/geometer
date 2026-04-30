#include "geometer/c_api.h"

#include <cstdlib>
#include <cstring>

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

} // namespace

GeometerStringResult geometer_step_hlr_projection_json(GeometerBuffer step_data,
                                                       const char* options_json)
{
    (void)step_data;
    (void)options_json;

    GeometerStringResult result;
    result.code = 90;
    result.value = nullptr;
    result.error = copy_c_string("STEP HLR projection is not implemented yet.");
    return result;
}

void geometer_free_string(char* value)
{
    std::free(value);
}
