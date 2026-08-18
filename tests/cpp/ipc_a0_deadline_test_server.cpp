#include "geometer/ipc_a0_server.h"

#include <chrono>

int main()
{
    geometer::ipc_a0::testing::ServerOptions options;
    options.shutdown_grace = std::chrono::milliseconds(150);
    options.execution_delay = std::chrono::seconds(5);
    return geometer::ipc_a0::testing::serve_stdio(options);
}
