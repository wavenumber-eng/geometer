#pragma once

#ifndef __EMSCRIPTEN__

#include <mutex>

namespace geometer
{

namespace testing
{

using NativeOperationExecutionObserver = void (*)(bool entering, void* context) noexcept;

// Test instrumentation only. Call this while no native operation is active.
void set_native_operation_execution_observer(NativeOperationExecutionObserver observer,
                                             void* context);

} // namespace testing

class NativeOperationExecutionGuard final
{
  public:
    NativeOperationExecutionGuard();
    ~NativeOperationExecutionGuard();

    NativeOperationExecutionGuard(const NativeOperationExecutionGuard&) = delete;
    NativeOperationExecutionGuard& operator=(const NativeOperationExecutionGuard&) = delete;
    NativeOperationExecutionGuard(NativeOperationExecutionGuard&&) = delete;
    NativeOperationExecutionGuard& operator=(NativeOperationExecutionGuard&&) = delete;

  private:
    std::unique_lock<std::mutex> lock_;
    testing::NativeOperationExecutionObserver observer_ = nullptr;
    void* observer_context_ = nullptr;
};

} // namespace geometer

#endif
