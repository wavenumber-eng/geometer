#include "geometer/ipc_a0_server.h"

#include "geometer/generated/contracts/contracts.h"
#include "geometer/ipc_a0_frame.h"
#include "geometer/operation_registry.h"
#include "geometer/operation_transport.h"
#include "geometer/version.h"

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_PrinterOStream.hxx>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace geometer::ipc_a0
{
namespace
{

// The process owns stdio while serving IPC. OCCT diagnostics must never enter
// stdout, including parser errors before a STEP model has transferred.
class IpcKernelDiagnostics
{
  public:
    IpcKernelDiagnostics() : saved_(Message::DefaultMessenger()->Printers())
    {
        auto printer = new Message_PrinterOStream("cerr", true);
        printer->SetToColorize(false);
        Message::DefaultMessenger()->ChangePrinters().Clear();
        Message::DefaultMessenger()->AddPrinter(printer);
    }
    ~IpcKernelDiagnostics()
    {
        Message::DefaultMessenger()->ChangePrinters() = saved_;
    }

  private:
    NCollection_Sequence<Handle(Message_Printer)> saved_;
};

constexpr std::size_t kMaxQueuedRequests = 8U;
constexpr std::size_t kMaxResidentBytes = 512U * 1024U * 1024U;
constexpr std::size_t kMaxPendingWriterBytes = 512U * 1024U * 1024U;

using JsonDocument = rapidjson::Document;

struct OutgoingFrame
{
    Frame frame;
    std::function<void()> flushed;
};

class FrameWriter
{
  public:
    explicit FrameWriter(std::FILE* stream, bool fail_after_welcome)
        : stream_(stream), fail_after_welcome_(fail_after_welcome), thread_(&FrameWriter::run, this)
    {
    }

    FrameWriter(const FrameWriter&) = delete;
    FrameWriter& operator=(const FrameWriter&) = delete;

    bool submit(OutgoingFrame outgoing)
    {
        const std::size_t bytes = encoded_size(outgoing.frame);
        if (bytes > kMaxPendingWriterBytes)
        {
            std::fprintf(stderr, "Geometer IPC output frame exceeds the bounded writer limit.\n");
            std::fflush(stderr);
            std::_Exit(2);
        }
        std::unique_lock lock(mutex_);
        condition_.wait(
            lock, [&]
            { return failed_ || stopping_ || pending_bytes_ <= kMaxPendingWriterBytes - bytes; });
        if (failed_ || stopping_)
        {
            return false;
        }
        pending_bytes_ += bytes;
        queue_.push_back(std::move(outgoing));
        condition_.notify_all();
        return true;
    }

    bool finish()
    {
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
            condition_.notify_all();
        }
        if (thread_.joinable())
        {
            thread_.join();
        }
        return !failed_;
    }

  private:
    void run()
    {
        for (;;)
        {
            OutgoingFrame outgoing;
            std::size_t bytes = 0;
            {
                std::unique_lock lock(mutex_);
                condition_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
                if (queue_.empty())
                {
                    return;
                }
                outgoing = std::move(queue_.front());
                queue_.pop_front();
                bytes = encoded_size(outgoing.frame);
            }
            std::string error;
            const bool wrote = !fail_after_welcome_ && write_frame(stream_, outgoing.frame, &error);
            if (!wrote)
            {
                if (fail_after_welcome_)
                {
                    error = "Injected writer failure after the IPC welcome.";
                }
                std::fprintf(stderr, "Geometer IPC stdout failure: %s\n", error.c_str());
                std::fflush(stderr);
                std::lock_guard lock(mutex_);
                failed_ = true;
                stopping_ = true;
                queue_.clear();
                pending_bytes_ = 0;
                condition_.notify_all();
                std::_Exit(2);
            }
            {
                std::lock_guard lock(mutex_);
                pending_bytes_ -= bytes;
                condition_.notify_all();
            }
            if (outgoing.flushed)
            {
                outgoing.flushed();
            }
        }
    }

    std::FILE* stream_;
    bool fail_after_welcome_ = false;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<OutgoingFrame> queue_;
    std::size_t pending_bytes_ = 0;
    bool failed_ = false;
    bool stopping_ = false;
    std::thread thread_;
};

enum class RequestState
{
    queued,
    active,
    terminal_pending,
};

struct QueuedRequest
{
    std::uint64_t id = 0;
    std::size_t resident_bytes = 0;
    std::string operation;
    std::string request_json;
    std::vector<Attachment> attachments;
};

struct SharedState
{
    SharedState(std::FILE* stream, const testing::ServerOptions& options)
        : shutdown_grace(options.shutdown_grace), execution_delay(options.execution_delay),
          exit_when_request_active(options.exit_when_request_active),
          writer(stream, options.fail_writer_after_welcome)
    {
    }

    std::mutex mutex;
    std::condition_variable condition;
    std::deque<QueuedRequest> queue;
    std::unordered_map<std::uint64_t, RequestState> requests;
    std::size_t resident_bytes = 0;
    std::uint64_t active_id = 0;
    bool draining = false;
    bool aborting = false;
    bool shutdown_rejections_submitted = false;
    bool active_at_shutdown = false;
    std::uint32_t rejected_at_shutdown = 0;
    std::atomic<bool> terminal_complete{false};
    std::chrono::milliseconds shutdown_grace;
    std::chrono::milliseconds execution_delay;
    bool exit_when_request_active = false;
    FrameWriter writer;
};

bool parse_json(const std::string& json, JsonDocument* document, std::string* error)
{
    document->Parse<rapidjson::kParseValidateEncodingFlag>(json.data(), json.size());
    if (document->HasParseError() || !document->IsObject())
    {
        *error = "Control or request envelope JSON is not one strict UTF-8 object.";
        return false;
    }
    return true;
}

bool valid_reason_object(const std::string& json, std::string* error)
{
    contracts::IpcReasonA0 reason;
    contracts::ContractError contract_error;
    if (!contracts::decode_json(reinterpret_cast<const unsigned char*>(json.data()), json.size(),
                                &reason, &contract_error))
    {
        *error = contract_error.message;
        return false;
    }
    return true;
}

bool parse_hello(const Frame& frame, std::string* error)
{
    if (frame.kind != FrameKind::hello || frame.request_id != 0 || !frame.attachments.empty())
    {
        *error = "The first IPC frame must be an attachment-free hello with request id zero.";
        return false;
    }
    contracts::IpcHelloA0 hello;
    contracts::ContractError contract_error;
    if (!contracts::decode_json(reinterpret_cast<const unsigned char*>(frame.json.data()),
                                frame.json.size(), &hello, &contract_error))
    {
        *error = contract_error.message;
        return false;
    }
    const bool supports_a0 =
        std::find(hello.protocols.begin(), hello.protocols.end(), "a0") != hello.protocols.end();
    if (!supports_a0)
    {
        *error = "IPC hello has no protocol identity in common with this server.";
        return false;
    }
    return true;
}

std::string welcome_json()
{
    contracts::ContractError error;
    contracts::IpcOperationCatalogA0 catalog;
    const std::string catalog_json = native_operation_catalog_json();
    if (!contracts::decode_json(reinterpret_cast<const unsigned char*>(catalog_json.data()),
                                catalog_json.size(), &catalog, &error))
    {
        std::fprintf(stderr, "Generated IPC operation catalog is invalid: %s\n",
                     error.message.c_str());
        std::_Exit(2);
    }
    contracts::IpcWelcomeA0 welcome;
    welcome.release_version = version_string();
    welcome.c_abi_generation = static_cast<std::uint32_t>(abi_version());
    welcome.operation_catalog = std::move(catalog);
    welcome.catalog_sha256 = normalized_contract_catalog_sha256();
    welcome.limits = {kMaxJsonBytes,           kMaxAttachmentCount, kMaxAttachmentTextBytes,
                      kMaxAttachmentTextBytes, kMaxAttachmentBytes, kMaxFrameBytes,
                      kMaxQueuedRequests,      kMaxResidentBytes,   kMaxResidentBytes,
                      kMaxPendingWriterBytes};
    welcome.capabilities = {"serialized_execution", "queue_only_cancellation", "raw_attachments"};
    std::string json;
    if (!contracts::encode_json(welcome, &json, &error))
    {
        std::fprintf(stderr, "Generated IPC welcome encoding failed: %s\n", error.message.c_str());
        std::_Exit(2);
    }
    return json;
}

contracts::DiagnosticA0 make_diagnostic(const char* code, bool retryable,
                                        const std::string& message, const std::string& operation,
                                        std::uint64_t id, const std::string& path = {})
{
    contracts::DiagnosticA0 diagnostic;
    diagnostic.code = code;
    diagnostic.category = contracts::DiagnosticCategory::transport;
    diagnostic.message = message;
    diagnostic.retryable = retryable;
    if (!operation.empty())
    {
        diagnostic.operation = operation;
    }
    diagnostic.request_id = std::to_string(id);
    if (!path.empty())
    {
        diagnostic.path = path;
    }
    return diagnostic;
}

Frame operation_failure(std::uint64_t id, const std::string& operation, const char* code,
                        bool retryable, const std::string& message, const std::string& path = {})
{
    contracts::OperationFailureA0 failure;
    failure.operation = operation;
    failure.diagnostics.push_back(make_diagnostic(code, retryable, message, operation, id, path));
    contracts::OperationOutcomeA0 outcome = std::move(failure);
    Frame frame;
    frame.kind = FrameKind::response;
    frame.request_id = id;
    contracts::ContractError error;
    if (!contracts::encode_json(outcome, &frame.json, &error))
    {
        frame.json = "{\"operation\":\"geometer.transport\",\"ok\":false,\"diagnostics\":[{"
                     "\"code\":\"geometer.transport.internal_error\",\"category\":\"transport\","
                     "\"message\":\"Failed to encode transport failure.\",\"retryable\":false}]}";
    }
    return frame;
}

template <typename T> std::string encode_control(const T& value)
{
    contracts::ContractError error;
    std::string json;
    if (!contracts::encode_json(value, &json, &error))
    {
        std::fprintf(stderr, "Generated IPC control encoding failed: %s\n", error.message.c_str());
        std::_Exit(2);
    }
    return json;
}

Frame protocol_error(std::uint64_t id, const std::string& message)
{
    contracts::IpcProtocolErrorA0 control;
    control.diagnostic =
        make_diagnostic("geometer.transport.protocol_error", false, message, "", id);
    return {FrameKind::protocol_error, id, encode_control(control), {}};
}

std::function<void()> terminal_callback(const std::shared_ptr<SharedState>& shared,
                                        std::uint64_t id)
{
    return [shared, id]
    {
        std::lock_guard lock(shared->mutex);
        shared->requests.erase(id);
        shared->condition.notify_all();
    };
}

bool submit_terminal(const std::shared_ptr<SharedState>& shared, Frame frame)
{
    const std::uint64_t id = frame.request_id;
    return shared->writer.submit({std::move(frame), terminal_callback(shared, id)});
}

bool parse_request(Frame* frame, QueuedRequest* request, std::string* diagnostic_code,
                   std::string* diagnostic_path, std::string* error)
{
    JsonDocument document;
    if (!parse_json(frame->json, &document, error))
    {
        *diagnostic_code = "geometer.contract.invalid_json";
        return false;
    }
    contracts::IpcRequestA0 envelope;
    contracts::ContractError contract_error;
    if (!contracts::decode_json(reinterpret_cast<const unsigned char*>(frame->json.data()),
                                frame->json.size(), &envelope, &contract_error))
    {
        *diagnostic_code = contract_error.code;
        *diagnostic_path = contract_error.path;
        *error = contract_error.message;
        return false;
    }
    if (!operation_request_value_matches(envelope.operation, envelope.request))
    {
        *diagnostic_code = "geometer.contract.operation_payload_mismatch";
        *diagnostic_path = "/request";
        *error = "The request payload does not match the declared operation.";
        return false;
    }
    const auto request_member = document.FindMember("request");
    if (request_member == document.MemberEnd())
    {
        *error = "The validated IPC request value is missing.";
        return false;
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!request_member->value.Accept(writer))
    {
        *error = "The validated IPC request value could not be encoded.";
        return false;
    }
    request->request_json.assign(buffer.GetString(), buffer.GetSize());
    request->id = frame->request_id;
    request->resident_bytes = encoded_size(*frame);
    request->operation = std::move(envelope.operation);
    request->attachments = std::move(frame->attachments);
    return true;
}

Frame execute_request(const QueuedRequest& request)
{
    std::vector<OperationAttachmentView> attachments;
    attachments.reserve(request.attachments.size());
    for (const auto& attachment : request.attachments)
    {
        attachments.push_back({attachment.name, attachment.media_type, attachment.data.data(),
                               attachment.data.size()});
    }
    OperationExecution execution;
    execute_native_operation(request.operation,
                             reinterpret_cast<const unsigned char*>(request.request_json.data()),
                             request.request_json.size(), attachments, &execution);
    Frame response;
    response.kind = FrameKind::response;
    response.request_id = request.id;
    contracts::ContractError error;
    if (!contracts::encode_json(execution.outcome, &response.json, &error))
    {
        return operation_failure(request.id, request.operation,
                                 "geometer.transport.response_encoding_failed", false,
                                 "The operation outcome could not be encoded.");
    }
    std::string validation_message;
    if (validate_operation_response(request.operation, response.json, execution.attachments,
                                    &validation_message) != OperationResponseValidationStatus::ok)
    {
        return operation_failure(request.id, request.operation,
                                 "geometer.transport.invalid_operation_response", false,
                                 validation_message);
    }
    for (auto& attachment : execution.attachments)
    {
        response.attachments.push_back({std::move(attachment.name),
                                        std::move(attachment.media_type),
                                        std::move(attachment.data)});
    }
    if (encoded_size(response) > kMaxFrameBytes)
    {
        return operation_failure(request.id, request.operation,
                                 "geometer.transport.response_limit_exceeded", false,
                                 "The encoded operation response exceeds the IPC frame limit.");
    }
    return response;
}

Frame shutdown_ack(bool active_request_completed, std::uint32_t rejected_queued_request_count)
{
    contracts::IpcShutdownAckA0 control;
    control.activeRequestCompleted = active_request_completed;
    control.rejectedQueuedRequestCount = rejected_queued_request_count;
    return {FrameKind::shutdown_ack, 0, encode_control(control), {}};
}

void worker_loop(const std::shared_ptr<SharedState>& shared)
{
    for (;;)
    {
        QueuedRequest request;
        {
            std::unique_lock lock(shared->mutex);
            shared->condition.wait(
                lock,
                [&] { return shared->aborting || !shared->queue.empty() || shared->draining; });
            if (shared->aborting)
            {
                return;
            }
            if (shared->queue.empty())
            {
                if (shared->draining)
                {
                    return;
                }
                continue;
            }
            request = std::move(shared->queue.front());
            shared->queue.pop_front();
            shared->active_id = request.id;
            shared->requests[request.id] = RequestState::active;
        }

        if (shared->exit_when_request_active)
        {
            std::fprintf(stderr, "Geometer IPC test server exiting with an active request.\n");
            std::fflush(stderr);
            std::_Exit(73);
        }
        if (shared->execution_delay.count() > 0)
        {
            std::fprintf(stderr, "Geometer IPC test server delaying an active request.\n");
            std::fflush(stderr);
            std::this_thread::sleep_for(shared->execution_delay);
        }

        Frame response = execute_request(request);
        const std::size_t resident_bytes = request.resident_bytes;
        std::string().swap(request.request_json);
        std::vector<Attachment>().swap(request.attachments);
        bool submit_ack = false;
        {
            std::unique_lock lock(shared->mutex);
            if (shared->draining)
            {
                shared->condition.wait(
                    lock,
                    [&] { return shared->shutdown_rejections_submitted || shared->aborting; });
            }
            if (shared->aborting)
            {
                return;
            }
            shared->resident_bytes -= resident_bytes;
            shared->requests[request.id] = RequestState::terminal_pending;
            if (!submit_terminal(shared, std::move(response)))
            {
                shared->aborting = true;
                shared->condition.notify_all();
                return;
            }
            shared->active_id = 0;
            submit_ack = shared->draining;
            if (submit_ack &&
                !shared->writer.submit(
                    {shutdown_ack(shared->active_at_shutdown, shared->rejected_at_shutdown), {}}))
            {
                shared->aborting = true;
                shared->condition.notify_all();
                return;
            }
            shared->condition.notify_all();
        }
        if (submit_ack)
        {
            return;
        }
    }
}

void start_deadline(const std::shared_ptr<SharedState>& shared,
                    std::chrono::steady_clock::time_point deadline)
{
    std::thread(
        [shared, deadline]
        {
            std::this_thread::sleep_until(deadline);
            if (!shared->terminal_complete.load())
            {
                std::fprintf(stderr, "Geometer IPC shutdown exceeded its A0 deadline.\n");
                std::fflush(stderr);
                std::_Exit(124);
            }
        })
        .detach();
}

void begin_shutdown(const std::shared_ptr<SharedState>& shared)
{
    std::vector<QueuedRequest> rejected;
    bool submit_ack_now = false;
    bool active_at_shutdown = false;
    std::uint32_t rejected_at_shutdown = 0;
    std::chrono::steady_clock::time_point deadline;
    {
        std::lock_guard lock(shared->mutex);
        if (shared->draining)
        {
            return;
        }
        shared->draining = true;
        deadline = std::chrono::steady_clock::now() + shared->shutdown_grace;
        shared->active_at_shutdown = shared->active_id != 0;
        while (!shared->queue.empty())
        {
            rejected.push_back(std::move(shared->queue.front()));
            shared->queue.pop_front();
        }
        shared->rejected_at_shutdown = static_cast<std::uint32_t>(rejected.size());
        submit_ack_now = shared->active_id == 0;
        active_at_shutdown = shared->active_at_shutdown;
        rejected_at_shutdown = shared->rejected_at_shutdown;
    }
    start_deadline(shared, deadline);
    for (auto& request : rejected)
    {
        const std::size_t resident_bytes = request.resident_bytes;
        std::string().swap(request.request_json);
        std::vector<Attachment>().swap(request.attachments);
        {
            std::lock_guard lock(shared->mutex);
            shared->resident_bytes -= resident_bytes;
            shared->requests[request.id] = RequestState::terminal_pending;
        }
        submit_terminal(shared, operation_failure(request.id, request.operation,
                                                  "geometer.transport.server_shutting_down", true,
                                                  "The server is shutting down before execution."));
    }
    {
        std::lock_guard lock(shared->mutex);
        shared->shutdown_rejections_submitted = true;
        shared->condition.notify_all();
    }
    if (submit_ack_now)
    {
        shared->writer.submit({shutdown_ack(active_at_shutdown, rejected_at_shutdown), {}});
    }
}

bool handle_request(const std::shared_ptr<SharedState>& shared, Frame frame,
                    std::string* fatal_error)
{
    if (frame.request_id == 0)
    {
        *fatal_error = "IPC request frames require a nonzero request id.";
        return false;
    }
    {
        std::lock_guard lock(shared->mutex);
        if (shared->requests.count(frame.request_id))
        {
            *fatal_error = "IPC request id is already outstanding.";
            return false;
        }
        shared->requests.emplace(frame.request_id, RequestState::terminal_pending);
    }

    QueuedRequest request;
    std::string diagnostic_code = "geometer.contract.invalid_request_envelope";
    std::string diagnostic_path;
    std::string request_error;
    if (!parse_request(&frame, &request, &diagnostic_code, &diagnostic_path, &request_error))
    {
        std::string operation = "geometer.transport.invalid_request";
        JsonDocument document;
        if (parse_json(frame.json, &document, &request_error))
        {
            const auto member = document.FindMember("operation");
            if (member != document.MemberEnd() && member->value.IsString())
            {
                operation.assign(member->value.GetString(), member->value.GetStringLength());
            }
        }
        submit_terminal(shared,
                        operation_failure(frame.request_id, operation, diagnostic_code.c_str(),
                                          false, request_error, diagnostic_path));
        return true;
    }

    bool saturated = false;
    {
        std::lock_guard lock(shared->mutex);
        saturated = shared->queue.size() >= kMaxQueuedRequests ||
                    request.resident_bytes > kMaxResidentBytes - shared->resident_bytes;
        if (!saturated)
        {
            shared->resident_bytes += request.resident_bytes;
            shared->requests[request.id] = RequestState::queued;
            shared->queue.push_back(std::move(request));
            shared->condition.notify_all();
        }
    }
    if (saturated)
    {
        submit_terminal(shared, operation_failure(frame.request_id, request.operation,
                                                  "geometer.transport.queue_full", true,
                                                  "The bounded IPC request queue is full."));
    }
    return true;
}

bool handle_cancel(const std::shared_ptr<SharedState>& shared, const Frame& frame,
                   std::string* fatal_error)
{
    if (frame.request_id == 0 || !frame.attachments.empty() ||
        !valid_reason_object(frame.json, fatal_error))
    {
        *fatal_error = "IPC cancel is not a valid attachment-free control frame.";
        return false;
    }
    std::optional<QueuedRequest> removed;
    RequestState state = RequestState::terminal_pending;
    bool known = false;
    {
        std::lock_guard lock(shared->mutex);
        const auto found = shared->requests.find(frame.request_id);
        known = found != shared->requests.end();
        if (known)
        {
            state = found->second;
        }
        if (known && state == RequestState::queued)
        {
            const auto queued = std::find_if(shared->queue.begin(), shared->queue.end(),
                                             [&](const QueuedRequest& request)
                                             { return request.id == frame.request_id; });
            if (queued != shared->queue.end())
            {
                removed = std::move(*queued);
                shared->queue.erase(queued);
                found->second = RequestState::terminal_pending;
            }
        }
    }
    if (removed.has_value())
    {
        const std::size_t resident_bytes = removed->resident_bytes;
        std::string().swap(removed->request_json);
        std::vector<Attachment>().swap(removed->attachments);
        {
            std::lock_guard lock(shared->mutex);
            shared->resident_bytes -= resident_bytes;
        }
        contracts::IpcCancelledA0 control;
        Frame response{FrameKind::cancelled, frame.request_id, encode_control(control), {}};
        submit_terminal(shared, std::move(response));
    }
    else
    {
        const bool active = known && state == RequestState::active;
        contracts::IpcCancelRejectedA0 control;
        control.diagnostic = make_diagnostic(active ? "geometer.transport.not_cancellable"
                                                    : "geometer.transport.unknown_request",
                                             false,
                                             active ? "The request is already executing."
                                                    : "The request is unknown or already terminal.",
                                             "", frame.request_id);
        Frame response{FrameKind::cancel_rejected, frame.request_id, encode_control(control), {}};
        shared->writer.submit({std::move(response), {}});
    }
    return true;
}

int fail_connection(const std::shared_ptr<SharedState>& shared, std::uint64_t id,
                    const std::string& message)
{
    shared->writer.submit({protocol_error(id, message), []
                           {
                               std::fflush(stderr);
                               std::_Exit(2);
                           }});
    {
        std::lock_guard lock(shared->mutex);
        shared->aborting = true;
        shared->condition.notify_all();
    }
    return 2;
}

int serve_stdio_impl(const testing::ServerOptions& options)
{
    const IpcKernelDiagnostics diagnostics;
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1 || _setmode(_fileno(stdout), _O_BINARY) == -1)
    {
        std::fprintf(stderr, "Failed to switch Geometer IPC stdio to binary mode.\n");
        return 2;
    }
#endif

    Frame hello;
    std::string error;
    const ReadStatus hello_status = read_frame(stdin, &hello, &error);
    if (hello_status != ReadStatus::ok || !parse_hello(hello, &error))
    {
        Frame response = protocol_error(0, error.empty() ? "IPC hello was not received." : error);
        write_frame(stdout, response, nullptr);
        return 2;
    }
    Frame welcome{FrameKind::welcome, 0, welcome_json(), {}};
    if (!write_frame(stdout, welcome, &error))
    {
        std::fprintf(stderr, "Failed to write Geometer IPC welcome: %s\n", error.c_str());
        return 2;
    }

    const auto shared = std::make_shared<SharedState>(stdout, options);
    std::thread worker(worker_loop, shared);
    int result = 0;
    for (;;)
    {
        Frame frame;
        const ReadStatus status = read_frame(stdin, &frame, &error);
        if (status == ReadStatus::end_of_stream)
        {
            begin_shutdown(shared);
            break;
        }
        if (status == ReadStatus::error)
        {
            result = fail_connection(shared, 0, error);
            break;
        }
        if (frame.kind == FrameKind::request)
        {
            if (!handle_request(shared, std::move(frame), &error))
            {
                result = fail_connection(shared, frame.request_id, error);
                break;
            }
        }
        else if (frame.kind == FrameKind::cancel)
        {
            if (!handle_cancel(shared, frame, &error))
            {
                result = fail_connection(shared, frame.request_id, error);
                break;
            }
        }
        else if (frame.kind == FrameKind::shutdown)
        {
            if (frame.request_id != 0 || !frame.attachments.empty() ||
                !valid_reason_object(frame.json, &error))
            {
                result =
                    fail_connection(shared, frame.request_id,
                                    "IPC shutdown is not a valid attachment-free control frame.");
                break;
            }
            begin_shutdown(shared);
            break;
        }
        else
        {
            result = fail_connection(shared, frame.request_id,
                                     "IPC frame kind is invalid in the running client direction.");
            break;
        }
    }

    if (worker.joinable())
    {
        worker.join();
    }
    if (!shared->writer.finish())
    {
        result = 2;
    }
    shared->terminal_complete.store(true);
    return result;
}

} // namespace

int serve_stdio()
{
    return serve_stdio_impl({});
}

namespace testing
{

int serve_stdio(const ServerOptions& options)
{
    return serve_stdio_impl(options);
}

} // namespace testing

} // namespace geometer::ipc_a0
