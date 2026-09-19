#include "geometer/c_api.h"

int main()
{
    const auto* bootstrap = &geometer_serve_stdio;
    return geometer_abi_version() > 0 && bootstrap != nullptr ? 0 : 1;
}
