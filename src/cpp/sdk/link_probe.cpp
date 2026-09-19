#include "geometer/c_api.h"

int main()
{
    return geometer_abi_version() > 0 ? 0 : 1;
}
