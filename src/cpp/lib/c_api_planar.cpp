#include "geometer/c_api.h"

#include "geometer/planar_solve.h"
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
