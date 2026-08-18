#include "geometer/ipc_a0_server.h"

int main()
{
    geometer::ipc_a0::testing::ServerOptions options;
    options.exit_when_request_active = true;
    return geometer::ipc_a0::testing::serve_stdio(options);
}
