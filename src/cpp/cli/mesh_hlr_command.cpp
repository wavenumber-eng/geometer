#include "mesh_hlr_command.h"

#include "geometer/operation_registry.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

namespace geometer::cli
{
namespace
{

constexpr const char* kOperation = "geometry.mesh_hlr_projection.a0";
constexpr const char* kMediaType = "application/vnd.wavenumber.geometer.indexed-triangle-mesh";

bool read_bytes(const std::string& path, std::vector<unsigned char>* bytes)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes->assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return input.good() || input.eof();
}

bool write_text(const std::string& path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary);
    if (!output)
        return false;
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    return output.good();
}

bool parse_options(const std::string& text, rapidjson::Document* document,
                   std::string* error_message)
{
    document->Parse(text.c_str());
    if (!document->HasParseError() && document->IsObject())
        return true;
    *error_message = "HLR options must be a JSON object";
    return false;
}

bool merged_options(const std::string& base_json, const std::string& override_json,
                    std::string* output, std::string* error_message)
{
    rapidjson::Document base;
    rapidjson::Document override_value;
    if (!parse_options(base_json, &base, error_message) ||
        !parse_options(override_json, &override_value, error_message))
        return false;

    rapidjson::Document merged;
    merged.SetObject();
    auto& allocator = merged.GetAllocator();
    const auto append_members = [&merged, &allocator](const rapidjson::Value& source)
    {
        for (auto member = source.MemberBegin(); member != source.MemberEnd(); ++member)
        {
            rapidjson::Value name(member->name, allocator);
            rapidjson::Value value(member->value, allocator);
            merged.RemoveMember(member->name.GetString());
            merged.AddMember(name, value, allocator);
        }
    };
    append_members(base);
    append_members(override_value);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    merged.Accept(writer);
    *output = buffer.GetString();
    return true;
}

std::string diagnostic_text(const contracts::OperationFailureA0& failure)
{
    std::string result;
    for (const contracts::DiagnosticA0& diagnostic : failure.diagnostics)
    {
        if (!result.empty())
            result += "; ";
        result += diagnostic.code;
        if (diagnostic.path.has_value())
            result += " at " + *diagnostic.path;
        result += ": " + diagnostic.message;
    }
    return result.empty() ? "Indexed-mesh HLR failed" : result;
}

} // namespace

int project_indexed_mesh_file(const std::string& input_path, const std::string& output_path,
                              const std::string& base_options_json,
                              const std::string& override_options_json, std::string* error_message)
{
    if (error_message == nullptr)
        return 2;

    std::vector<unsigned char> packet;
    if (!read_bytes(input_path, &packet))
    {
        *error_message = "failed reading " + input_path;
        return 1;
    }

    std::string request_json;
    if (!merged_options(base_options_json, override_options_json, &request_json, error_message))
        return 2;

    OperationExecution execution;
    execute_operation(kOperation, reinterpret_cast<const unsigned char*>(request_json.data()),
                      request_json.size(), {{"mesh", kMediaType, packet.data(), packet.size()}},
                      &execution);
    if (const auto* failure = std::get_if<contracts::OperationFailureA0>(&execution.outcome))
    {
        *error_message = diagnostic_text(*failure);
        return 2;
    }
    const auto* success = std::get_if<contracts::OperationSuccessA0>(&execution.outcome);
    if (success == nullptr)
    {
        *error_message = "Indexed-mesh HLR returned an invalid operation outcome";
        return 1;
    }
    const auto* result = std::get_if<contracts::HlrProjectionResultA0>(&success->result);
    if (result == nullptr)
    {
        *error_message = "Indexed-mesh HLR returned an incompatible result";
        return 1;
    }

    contracts::ContractError contract_error;
    std::string json;
    if (!contracts::encode_json(*result, &json, &contract_error))
    {
        *error_message = contract_error.message;
        return 1;
    }
    if (!write_text(output_path, json))
    {
        *error_message = "failed writing " + output_path;
        return 1;
    }
    error_message->clear();
    return 0;
}

} // namespace geometer::cli
