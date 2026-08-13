#include "geometer/ipc_a0_server.h"

#include "geometer/generated/contracts/contracts.h"
#include "geometer/ipc_a0_frame.h"
#include "geometer/operation_registry.h"
#include "geometer/operation_transport.h"
#include "geometer/version.h"

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
#include <unordered_set>
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

constexpr std::size_t kMaxQueuedRequests = 8U;
constexpr std::size_t kMaxResidentBytes = 512U * 1024U * 1024U;
constexpr std::size_t kMaxPendingWriterBytes = 512U * 1024U * 1024U;
constexpr auto kShutdownGrace = std::chrono::seconds(30);

using JsonDocument = rapidjson::Document;
using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

struct OutgoingFrame
{
    Frame frame;
    std::function<void()> flushed;
};

class FrameWriter
{
  public:
    explicit FrameWriter(std::FILE* stream) : stream_(stream), thread_(&FrameWriter::run, this) {}

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
            if (!write_frame(stream_, outgoing.frame, &error))
            {
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
    explicit SharedState(std::FILE* stream) : writer(stream) {}

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

bool validate_members(const JsonDocument& document, const std::unordered_set<std::string>& allowed,
                      const std::unordered_set<std::string>& required, std::string* error)
{
    std::unordered_set<std::string> found;
    for (auto member = document.MemberBegin(); member != document.MemberEnd(); ++member)
    {
        const std::string name(member->name.GetString(), member->name.GetStringLength());
        if (!allowed.count(name))
        {
            *error = "JSON object contains an unknown field.";
            return false;
        }
        if (!found.insert(name).second)
        {
            *error = "JSON object contains a duplicate field.";
            return false;
        }
    }
    for (const auto& name : required)
    {
        if (!found.count(name))
        {
            *error = "JSON object is missing a required field.";
            return false;
        }
    }
    return true;
}

std::string json_stringify(const rapidjson::Value& value)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    value.Accept(writer);
    return {buffer.GetString(), buffer.GetSize()};
}

bool valid_reason_object(const std::string& json, std::string* error)
{
    JsonDocument document;
    if (!parse_json(json, &document, error) || !validate_members(document, {"reason"}, {}, error))
    {
        return false;
    }
    const auto reason = document.FindMember("reason");
    return reason == document.MemberEnd() || reason->value.IsString();
}

bool parse_hello(const Frame& frame, std::string* error)
{
    if (frame.kind != FrameKind::hello || frame.request_id != 0 || !frame.attachments.empty())
    {
        *error = "The first IPC frame must be an attachment-free hello with request id zero.";
        return false;
    }
    JsonDocument document;
    if (!parse_json(frame.json, &document, error) ||
        !validate_members(document, {"client_name", "client_version", "protocols", "capabilities"},
                          {"client_name", "client_version", "protocols"}, error))
    {
        return false;
    }
    const auto name = document.FindMember("client_name");
    const auto version = document.FindMember("client_version");
    const auto protocols = document.FindMember("protocols");
    if (!name->value.IsString() || name->value.GetStringLength() == 0 ||
        !version->value.IsString() || version->value.GetStringLength() == 0 ||
        !protocols->value.IsArray())
    {
        *error = "IPC hello fields have invalid types or empty identities.";
        return false;
    }
    bool supports_a0 = false;
    for (const auto& protocol : protocols->value.GetArray())
    {
        if (!protocol.IsString())
        {
            *error = "IPC hello protocols must be strings.";
            return false;
        }
        supports_a0 =
            supports_a0 || std::string(protocol.GetString(), protocol.GetStringLength()) == "a0";
    }
    const auto capabilities = document.FindMember("capabilities");
    if (capabilities != document.MemberEnd())
    {
        if (!capabilities->value.IsArray())
        {
            *error = "IPC hello capabilities must be an array of strings.";
            return false;
        }
        for (const auto& capability : capabilities->value.GetArray())
        {
            if (!capability.IsString())
            {
                *error = "IPC hello capabilities must be an array of strings.";
                return false;
            }
        }
    }
    if (!supports_a0)
    {
        *error = "IPC hello has no protocol identity in common with this server.";
        return false;
    }
    return true;
}

std::string welcome_json()
{
    JsonDocument catalog;
    catalog.Parse(operation_catalog_json());
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("release_version");
    writer.String(version_string());
    writer.Key("c_abi_generation");
    writer.Uint(static_cast<unsigned int>(abi_version()));
    writer.Key("ipc");
    writer.String("a0");
    writer.Key("catalog_sha256");
    writer.String(normalized_contract_catalog_sha256());
    writer.Key("operation_catalog");
    catalog.Accept(writer);
    writer.Key("limits");
    writer.StartObject();
    writer.Key("json_bytes");
    writer.Uint64(kMaxJsonBytes);
    writer.Key("attachment_count");
    writer.Uint64(kMaxAttachmentCount);
    writer.Key("attachment_name_bytes");
    writer.Uint64(kMaxAttachmentTextBytes);
    writer.Key("attachment_media_type_bytes");
    writer.Uint64(kMaxAttachmentTextBytes);
    writer.Key("attachment_bytes");
    writer.Uint64(kMaxAttachmentBytes);
    writer.Key("frame_bytes");
    writer.Uint64(kMaxFrameBytes);
    writer.Key("queued_requests");
    writer.Uint64(kMaxQueuedRequests);
    writer.Key("queued_bytes");
    writer.Uint64(kMaxResidentBytes);
    writer.Key("resident_request_bytes");
    writer.Uint64(kMaxResidentBytes);
    writer.Key("pending_writer_bytes");
    writer.Uint64(kMaxPendingWriterBytes);
    writer.EndObject();
    writer.Key("capabilities");
    writer.StartArray();
    writer.String("serialized_execution");
    writer.String("queue_only_cancellation");
    writer.String("raw_attachments");
    writer.EndArray();
    writer.EndObject();
    return {buffer.GetString(), buffer.GetSize()};
}

contracts::DiagnosticA0 make_diagnostic(const char* code, bool retryable,
                                        const std::string& message, const std::string& operation,
                                        std::uint64_t id)
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
    return diagnostic;
}

Frame operation_failure(std::uint64_t id, const std::string& operation, const char* code,
                        bool retryable, const std::string& message)
{
    contracts::OperationFailureA0 failure;
    failure.operation = operation;
    failure.diagnostics.push_back(make_diagnostic(code, retryable, message, operation, id));
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

std::string control_diagnostic_json(const char* status, const char* code,
                                    const std::string& message, std::uint64_t id)
{
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("status");
    writer.String(status);
    writer.Key("diagnostic");
    writer.StartObject();
    writer.Key("code");
    writer.String(code);
    writer.Key("category");
    writer.String("transport");
    writer.Key("message");
    writer.String(message.c_str(), static_cast<rapidjson::SizeType>(message.size()));
    writer.Key("retryable");
    writer.Bool(false);
    writer.Key("request_id");
    const std::string request_id = std::to_string(id);
    writer.String(request_id.c_str(), static_cast<rapidjson::SizeType>(request_id.size()));
    writer.EndObject();
    writer.EndObject();
    return {buffer.GetString(), buffer.GetSize()};
}

Frame protocol_error(std::uint64_t id, const std::string& message)
{
    return {
        FrameKind::protocol_error,
        id,
        control_diagnostic_json("protocol_error", "geometer.transport.protocol_error", message, id),
        {}};
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

bool parse_request(Frame* frame, QueuedRequest* request, std::string* error)
{
    JsonDocument document;
    if (!parse_json(frame->json, &document, error) ||
        !validate_members(document, {"operation", "request"}, {"operation", "request"}, error))
    {
        return false;
    }
    const auto operation = document.FindMember("operation");
    const auto body = document.FindMember("request");
    if (!operation->value.IsString() || operation->value.GetStringLength() == 0 ||
        operation->value.GetStringLength() > 128U || !body->value.IsObject())
    {
        *error = "IPC request envelope has an invalid operation or request field.";
        return false;
    }
    request->id = frame->request_id;
    request->resident_bytes = encoded_size(*frame);
    request->operation.assign(operation->value.GetString(), operation->value.GetStringLength());
    request->request_json = json_stringify(body->value);
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
    execute_operation(request.operation,
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
    rapidjson::StringBuffer buffer;
    JsonWriter writer(buffer);
    writer.StartObject();
    writer.Key("status");
    writer.String("complete");
    writer.Key("activeRequestCompleted");
    writer.Bool(active_request_completed);
    writer.Key("rejectedQueuedRequestCount");
    writer.Uint(rejected_queued_request_count);
    writer.EndObject();
    return {FrameKind::shutdown_ack, 0, {buffer.GetString(), buffer.GetSize()}, {}};
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
                std::fprintf(stderr, "Geometer IPC shutdown exceeded the 30 second A0 deadline.\n");
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
        deadline = std::chrono::steady_clock::now() + kShutdownGrace;
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
    std::string request_error;
    if (!parse_request(&frame, &request, &request_error))
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
        submit_terminal(shared, operation_failure(frame.request_id, operation,
                                                  "geometer.contract.invalid_request_envelope",
                                                  false, request_error));
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
        Frame response{FrameKind::cancelled, frame.request_id, "{\"status\":\"cancelled\"}", {}};
        submit_terminal(shared, std::move(response));
    }
    else
    {
        const bool active = known && state == RequestState::active;
        Frame response{FrameKind::cancel_rejected,
                       frame.request_id,
                       control_diagnostic_json("rejected",
                                               active ? "geometer.transport.not_cancellable"
                                                      : "geometer.transport.unknown_request",
                                               active
                                                   ? "The request is already executing."
                                                   : "The request is unknown or already terminal.",
                                               frame.request_id),
                       {}};
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

} // namespace

int serve_stdio()
{
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

    const auto shared = std::make_shared<SharedState>(stdout);
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

} // namespace geometer::ipc_a0
