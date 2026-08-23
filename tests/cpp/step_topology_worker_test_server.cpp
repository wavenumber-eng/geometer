#include "geometer/step_topology_session.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{

namespace fs = std::filesystem;

std::vector<unsigned char> read_bytes(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("failed to open STEP fixture");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

fs::path private_temp_directory()
{
    const char* value = std::getenv("GEOMETER_TOPOLOGY_WORKER_TEMP");
    if (value == nullptr || *value == '\0')
    {
        throw std::runtime_error("private worker temp directory is not configured");
    }
    return value;
}

void wait_for_supervisor_gate()
{
    const char* value = std::getenv("GEOMETER_TOPOLOGY_WORKER_START_GATE");
    if (value == nullptr || *value == '\0')
    {
        throw std::runtime_error("worker start gate is not configured");
    }
    const fs::path gate(value);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!fs::is_regular_file(gate))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            throw std::runtime_error("worker start gate timed out");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void write_marker(const std::string& value)
{
    const fs::path directory = private_temp_directory();
    std::ofstream marker(directory / "worker.marker", std::ios::binary | std::ios::trunc);
    if (!marker || !(marker << value))
    {
        throw std::runtime_error("failed to write private worker marker");
    }
}

void hold_descendant_marker()
{
    std::ofstream marker(private_temp_directory() / "descendant.marker",
                         std::ios::binary | std::ios::trunc);
    if (!marker || !(marker << "holding\n" << std::flush))
    {
        throw std::runtime_error("failed to open descendant worker marker");
    }
    for (;;)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void spawn_descendant(const fs::path& executable)
{
#ifdef _WIN32
    std::wstring command = L"\"" + executable.wstring() + L"\" descendant-hold";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &startup, &process))
    {
        throw std::runtime_error("failed to spawn descendant worker");
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
#else
    const pid_t child = fork();
    if (child < 0)
    {
        throw std::runtime_error("failed to fork descendant worker");
    }
    if (child == 0)
    {
        const std::string path = executable.string();
        execl(path.c_str(), path.c_str(), "descendant-hold", static_cast<char*>(nullptr));
        _exit(127);
    }
#endif
    const fs::path marker = private_temp_directory() / "descendant.marker";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!fs::is_regular_file(marker))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            throw std::runtime_error("descendant worker marker timed out");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

std::unique_ptr<geometer::StepTopologySession> open_session(const fs::path& path)
{
    const std::vector<unsigned char> bytes = read_bytes(path);
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    const int code =
        geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session, &status);
    if (code != 0)
    {
        throw std::runtime_error("failed to open contained topology session: " + status.message);
    }
    return session;
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        wait_for_supervisor_gate();
        if (argc == 2 && std::string(argv[1]) == "descendant-hold")
        {
            hold_descendant_marker();
        }
        if (argc == 3 && std::string(argv[1]) == "open-once")
        {
            std::unique_ptr<geometer::StepTopologySession> session = open_session(argv[2]);
            write_marker(session->info().session_handle);
            std::cout << session->info().session_handle << '\n';
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "open-hold")
        {
            std::unique_ptr<geometer::StepTopologySession> session = open_session(argv[2]);
            write_marker(session->info().session_handle);
            std::cout << session->info().session_handle << '\n' << std::flush;
            for (;;)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        if (argc == 3 && std::string(argv[1]) == "open-hold-tree")
        {
            std::unique_ptr<geometer::StepTopologySession> session = open_session(argv[2]);
            write_marker(session->info().session_handle);
            spawn_descendant(fs::absolute(argv[0]));
            std::cout << session->info().session_handle << '\n' << std::flush;
            for (;;)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        if (argc == 3 && std::string(argv[1]) == "allocate")
        {
            const std::size_t requested = static_cast<std::size_t>(std::stoull(argv[2]));
            std::vector<std::vector<unsigned char>> allocations;
            std::size_t allocated = 0;
            constexpr std::size_t chunk_size = 1024U * 1024U;
            while (allocated < requested)
            {
                allocations.emplace_back(std::min(chunk_size, requested - allocated), 0xa5U);
                allocated += allocations.back().size();
            }
            std::cout << allocated << '\n';
            return 0;
        }
        std::cerr << "usage: step_topology_worker_test_server "
                     "open-once STEP | open-hold STEP | open-hold-tree STEP | allocate BYTES\n";
        return 2;
    }
    catch (const std::bad_alloc&)
    {
        std::cerr << "worker allocation exceeded its OS memory ceiling\n";
        return 42;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
