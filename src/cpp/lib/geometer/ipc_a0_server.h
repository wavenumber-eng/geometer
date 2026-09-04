#pragma once

#include <chrono>

namespace geometer::ipc_a0
{

int serve_stdio();

namespace testing
{

struct ServerOptions
{
    std::chrono::milliseconds shutdown_grace{std::chrono::seconds(30)};
    std::chrono::milliseconds execution_delay{0};
    bool exit_when_request_active = false;
    bool fail_writer_after_welcome = false;
};

int serve_stdio(const ServerOptions& options);

} // namespace testing

} // namespace geometer::ipc_a0
