#include "geometer/native_operation_execution_gate.h"

#ifndef __EMSCRIPTEN__

namespace geometer
{
namespace
{

std::mutex& execution_mutex()
{
    static std::mutex mutex;
    return mutex;
}

testing::NativeOperationExecutionObserver& execution_observer()
{
    static testing::NativeOperationExecutionObserver observer = nullptr;
    return observer;
}

void*& execution_observer_context()
{
    static void* context = nullptr;
    return context;
}

} // namespace

NativeOperationExecutionGuard::NativeOperationExecutionGuard() : lock_(execution_mutex())
{
    observer_ = execution_observer();
    observer_context_ = execution_observer_context();
    if (observer_ != nullptr)
        observer_(true, observer_context_);
}

NativeOperationExecutionGuard::~NativeOperationExecutionGuard()
{
    if (observer_ != nullptr)
        observer_(false, observer_context_);
}

namespace testing
{

void set_native_operation_execution_observer(NativeOperationExecutionObserver observer,
                                             void* context)
{
    std::lock_guard lock(execution_mutex());
    execution_observer_context() = context;
    execution_observer() = observer;
}

} // namespace testing
} // namespace geometer

#endif
