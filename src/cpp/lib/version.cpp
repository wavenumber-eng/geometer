#include "geometer/version.h"

#include "geometer/version_config.h"

namespace geometer
{

const Version& version()
{
    static const Version value{
        GEOMETER_CONFIG_VERSION_MAJOR,  GEOMETER_CONFIG_VERSION_MINOR,
        GEOMETER_CONFIG_VERSION_PATCH,  GEOMETER_CONFIG_ABI_VERSION,
        GEOMETER_CONFIG_VERSION_STRING,
    };
    return value;
}

const char* version_string()
{
    return version().string;
}

int version_major()
{
    return version().major;
}

int version_minor()
{
    return version().minor;
}

int version_patch()
{
    return version().patch;
}

int abi_version()
{
    return version().abi;
}

} // namespace geometer
