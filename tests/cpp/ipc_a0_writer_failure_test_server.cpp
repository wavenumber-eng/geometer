#include "geometer/ipc_a0_server.h"

int main()
{
    geometer::ipc_a0::testing::ServerOptions options;
    options.fail_writer_after_welcome = true;
    return geometer::ipc_a0::testing::serve_stdio(options);
}
