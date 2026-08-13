// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace geometer::contracts
{

struct ContractError
{
    std::string code;
    std::string path;
    std::string message;
};

enum class DiagnosticCategory
{
    transport,
    contract,
    operation,
};

struct DiagnosticA0
{
    std::string code{};
    DiagnosticCategory category{};
    std::string message{};
    bool retryable{};
    std::optional<std::string> path{};
    std::optional<std::string> operation{};
    std::optional<std::string> request_id{};
};

struct IpcAttachmentDeclarationA0
{
    std::string name{};
    bool required{};
    std::vector<std::string> media_types{};
    std::uint32_t max_bytes{};
};

struct IpcAttachmentOffsetsWasm32A0
{
    std::uint32_t struct_size{};
    std::uint32_t flags{};
    std::uint32_t name{};
    std::uint32_t name_size{};
    std::uint32_t media_type{};
    std::uint32_t media_type_size{};
    std::uint32_t data{};
    std::uint32_t data_size{};
    std::uint32_t reserved0{};
};

struct IpcAttachmentLayoutWasm32A0
{
    std::uint32_t size{};
    IpcAttachmentOffsetsWasm32A0 offsets{};
};

struct IpcAttachmentOffsetsPointer64A0
{
    std::uint32_t struct_size{};
    std::uint32_t flags{};
    std::uint32_t name{};
    std::uint32_t name_size{};
    std::uint32_t media_type{};
    std::uint32_t media_type_size{};
    std::uint32_t data{};
    std::uint32_t data_size{};
    std::uint32_t reserved0{};
};

struct IpcAttachmentLayoutPointer64A0
{
    std::uint32_t size{};
    IpcAttachmentOffsetsPointer64A0 offsets{};
};

struct IpcAttachmentDescriptorA0
{
    IpcAttachmentLayoutWasm32A0 wasm32{};
    IpcAttachmentLayoutPointer64A0 pointer64{};
};

struct IpcCancelledA0
{
    std::string status = "cancelled";
};

struct IpcCancelRejectedA0
{
    std::string status = "rejected";
    DiagnosticA0 diagnostic{};
};

struct IpcEffectiveLimitsA0
{
    std::uint32_t json_bytes{};
    std::uint32_t attachment_count{};
    std::uint32_t attachment_name_bytes{};
    std::uint32_t attachment_media_type_bytes{};
    std::uint32_t attachment_bytes{};
    std::uint32_t frame_bytes{};
    std::uint32_t queued_requests{};
    std::uint32_t queued_bytes{};
    std::uint32_t resident_request_bytes{};
    std::uint32_t pending_writer_bytes{};
};

struct IpcGenericAbiLimitsA0
{
    std::uint32_t operation_id_bytes{};
    std::uint32_t request_json_bytes{};
    std::uint32_t response_json_bytes{};
    std::uint32_t attachment_count{};
    std::uint32_t attachment_name_bytes{};
    std::uint32_t attachment_media_type_bytes{};
    std::uint32_t attachment_bytes{};
    std::uint32_t aggregate_attachment_bytes_native{};
    std::uint32_t aggregate_attachment_bytes_wasm{};
};

struct IpcHelloA0
{
    std::string client_name{};
    std::string client_version{};
    std::vector<std::string> protocols{};
    std::optional<std::vector<std::string>> capabilities{};
};

struct IpcOperationDeclarationA0
{
    std::string identity{};
    std::string request_contract{};
    std::string result_contract{};
    std::vector<IpcAttachmentDeclarationA0> input_attachments{};
    std::vector<IpcAttachmentDeclarationA0> output_attachments{};
};

struct IpcOperationCatalogA0
{
    std::string catalog = "wn.geometer.operation_catalog.a0";
    std::string generic_abi = "a0";
    std::string release_version{};
    std::uint32_t c_abi_generation{};
    std::vector<IpcOperationDeclarationA0> operations{};
    IpcAttachmentDescriptorA0 attachment_descriptor{};
    IpcGenericAbiLimitsA0 limits{};
};

struct IpcProtocolErrorA0
{
    std::string status = "protocol_error";
    DiagnosticA0 diagnostic{};
};

struct IpcReasonA0
{
    std::optional<std::string> reason{};
};

enum class ModelFormat
{
    step,
};

using Matrix4x4 = std::vector<double>;

struct ModelBoundsOptionsA0
{
    std::optional<ModelFormat> format{};
    std::optional<Matrix4x4> model_transform{};
};

struct IpcRequestA0
{
    std::string operation{};
    ModelBoundsOptionsA0 request{};
};

struct IpcShutdownAckA0
{
    std::string status = "complete";
    bool activeRequestCompleted{};
    std::uint32_t rejectedQueuedRequestCount{};
};

struct IpcWelcomeA0
{
    std::string release_version{};
    std::uint32_t c_abi_generation{};
    std::string ipc = "a0";
    std::string catalog_sha256{};
    IpcOperationCatalogA0 operation_catalog{};
    IpcEffectiveLimitsA0 limits{};
    std::vector<std::string> capabilities{};
};

struct ModelBoundsSource
{
    ModelFormat format{};
    std::string hash{};
};

using Vector3 = std::vector<double>;

struct ModelBoundsValues
{
    Vector3 min{};
    Vector3 max{};
    Vector3 size{};
    Vector3 center{};
};

struct ModelBoundsTimings
{
    double model_read_ms{};
    double bounds_ms{};
};

struct ModelBoundsResultA0
{
    std::string schema = "geometry.model_bounds.a0";
    std::string units = "mm";
    ModelBoundsSource source{};
    ModelBoundsValues bounds{};
    ModelBoundsTimings timings{};
};

struct OperationFailureA0
{
    std::string operation{};
    bool ok = false;
    std::vector<DiagnosticA0> diagnostics{};
};

using OperationResultValueA0 = std::variant<ModelBoundsResultA0>;

struct OperationSuccessA0
{
    std::string operation{};
    bool ok = true;
    OperationResultValueA0 result{};
};

using OperationOutcomeA0 = std::variant<OperationSuccessA0, OperationFailureA0>;

bool decode_json(const unsigned char* data, std::size_t size, DiagnosticA0* value,
                 ContractError* error = nullptr);
bool encode_json(const DiagnosticA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcCancelledA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcCancelledA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcCancelRejectedA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcCancelRejectedA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcHelloA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcHelloA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcOperationCatalogA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcOperationCatalogA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcProtocolErrorA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcProtocolErrorA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcReasonA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcReasonA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcRequestA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcRequestA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcShutdownAckA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcShutdownAckA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, IpcWelcomeA0* value,
                 ContractError* error = nullptr);
bool encode_json(const IpcWelcomeA0& value, std::string* json, ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsOptionsA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelBoundsOptionsA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, ModelBoundsResultA0* value,
                 ContractError* error = nullptr);
bool encode_json(const ModelBoundsResultA0& value, std::string* json,
                 ContractError* error = nullptr);

bool decode_json(const unsigned char* data, std::size_t size, OperationOutcomeA0* value,
                 ContractError* error = nullptr);
bool encode_json(const OperationOutcomeA0& value, std::string* json,
                 ContractError* error = nullptr);

} // namespace geometer::contracts
