#include "geometer/c_api.h"
#include "geometer/native_operation_execution_gate.h"
#include "geometer/operation_registry.h"

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct Observation
{
    std::mutex mutex;
    std::condition_variable condition;
    int entered = 0;
    int active = 0;
    int maximum_active = 0;
    bool release_first = false;
    bool second_started = false;
    bool second_finished = false;
};

void observe_execution(bool entering, void* context) noexcept
{
    auto& observation = *static_cast<Observation*>(context);
    std::unique_lock lock(observation.mutex);
    if (entering)
    {
        ++observation.entered;
        ++observation.active;
        observation.maximum_active = std::max(observation.maximum_active, observation.active);
        observation.condition.notify_all();
        if (observation.entered == 1)
            observation.condition.wait(lock, [&observation] { return observation.release_first; });
        return;
    }
    --observation.active;
    observation.condition.notify_all();
}

void native_adapters_share_one_lane()
{
    Observation observation;
    geometer::testing::set_native_operation_execution_observer(observe_execution, &observation);

    const std::string direct_operation = "geometry.not_implemented.a0";
    const std::string direct_request = "{}";
    geometer::OperationExecution direct_execution;
    std::thread direct_thread(
        [&]
        {
            geometer::execute_native_operation(
                direct_operation, reinterpret_cast<const unsigned char*>(direct_request.data()),
                direct_request.size(), {}, &direct_execution);
        });

    {
        std::unique_lock lock(observation.mutex);
        observation.condition.wait(lock, [&observation] { return observation.entered == 1; });
    }

    const std::string c_operation = "geometry.step_topology.open.a0";
    const std::string c_request = R"({"schema":"geometry.step_topology.open.request.a0"})";
    GeometerOperationResult* c_result = nullptr;
    char* c_error = nullptr;
    int c_code = -1;
    std::thread c_thread(
        [&]
        {
            {
                std::lock_guard lock(observation.mutex);
                observation.second_started = true;
            }
            observation.condition.notify_all();
            c_code = geometer_operation_execute(
                c_operation.data(), static_cast<std::uint32_t>(c_operation.size()),
                reinterpret_cast<const unsigned char*>(c_request.data()),
                static_cast<std::uint32_t>(c_request.size()), nullptr, 0U, &c_result, &c_error);
            {
                std::lock_guard lock(observation.mutex);
                observation.second_finished = true;
            }
            observation.condition.notify_all();
        });

    {
        std::unique_lock lock(observation.mutex);
        observation.condition.wait(lock, [&observation] { return observation.second_started; });
        require(observation.entered == 1,
                "the second adapter entered while the first held the execution lane");
        observation.release_first = true;
        observation.condition.notify_all();
        observation.condition.wait(
            lock,
            [&observation] { return observation.entered == 2 || observation.second_finished; });
        require(observation.entered == 2, "the native C ABI bypassed the common execution lane");
    }

    direct_thread.join();
    c_thread.join();
    geometer::testing::set_native_operation_execution_observer(nullptr, nullptr);

    require(observation.maximum_active == 1 && observation.active == 0,
            "native adapter executions overlapped or failed to leave the lane");
    require(c_code == GEOMETER_OPERATION_ABI_OK && c_result != nullptr && c_error == nullptr,
            "the native C ABI did not return its governed contract failure");
    geometer_operation_result_free(c_result);
}

void exceptions_release_the_lane()
{
    try
    {
        geometer::NativeOperationExecutionGuard guard;
        throw std::runtime_error("expected test exception");
    }
    catch (const std::runtime_error&)
    {
    }

    geometer::NativeOperationExecutionGuard reacquired;
}

void focused_c_operations_use_the_lane()
{
    Observation observation;
    observation.release_first = true;
    geometer::testing::set_native_operation_execution_observer(observe_execution, &observation);

    const unsigned char invalid_request = 0U;
    unsigned char* value = nullptr;
    std::size_t value_size = 0U;
    char* error = nullptr;
    const int code =
        geometer_planar_batch_solve_bytes(&invalid_request, 1U, &value, &value_size, &error);

    geometer::testing::set_native_operation_execution_observer(nullptr, nullptr);
    require(code != 0 && value == nullptr && value_size == 0U && error != nullptr,
            "focused C operation did not return its expected invalid-packet failure");
    require(observation.entered == 1 && observation.maximum_active == 1 && observation.active == 0,
            "focused C operation bypassed or failed to leave the native execution lane");
    geometer_free_string(error);
}

void catalogs_do_not_acquire_the_lane()
{
    std::mutex mutex;
    std::condition_variable condition;
    bool entered = false;
    bool release = false;
    std::thread holder(
        [&]
        {
            geometer::NativeOperationExecutionGuard guard;
            std::unique_lock lock(mutex);
            entered = true;
            condition.notify_all();
            condition.wait(lock, [&release] { return release; });
        });
    {
        std::unique_lock lock(mutex);
        condition.wait(lock, [&entered] { return entered; });
    }

    char* catalog = nullptr;
    char* error = nullptr;
    require(geometer_operation_catalog_json(&catalog, &error) == GEOMETER_OPERATION_ABI_OK &&
                catalog != nullptr && error == nullptr,
            "catalog access unexpectedly waited for or failed under the execution lane");
    geometer_free_string(catalog);

    {
        std::lock_guard lock(mutex);
        release = true;
    }
    condition.notify_all();
    holder.join();
}

} // namespace

int main()
{
    try
    {
        native_adapters_share_one_lane();
        exceptions_release_the_lane();
        focused_c_operations_use_the_lane();
        catalogs_do_not_acquire_the_lane();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
