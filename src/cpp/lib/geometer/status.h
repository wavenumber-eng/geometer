#pragma once

#include <string>

namespace geometer
{

struct Status
{
    int code = 0;
    std::string message;

    bool ok() const
    {
        return code == 0;
    }
};

} // namespace geometer
