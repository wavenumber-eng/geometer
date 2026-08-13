// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.
#pragma once

#include <cstddef>
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

using Matrix4x4 = std::vector<double>;

enum class ModelFormat
{
    step,
};

struct ModelBoundsOptionsA0
{
    std::optional<ModelFormat> format{};
    std::optional<Matrix4x4> model_transform{};
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
