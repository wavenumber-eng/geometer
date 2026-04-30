#pragma once

namespace geometer
{

struct Version
{
    int major = 0;
    int minor = 0;
    int patch = 0;
    int abi = 0;
    const char* string = "";
};

const Version& version();
const char* version_string();
int version_major();
int version_minor();
int version_patch();
int abi_version();

} // namespace geometer
