#include "geometer.h"
#ifndef __EMSCRIPTEN__
#include "geometer/ipc_a0_server.h"
#endif

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

// OCCT static globals can segfault during atexit teardown.
// Use _Exit to skip destructors after successful completion.
[[noreturn]] static void clean_exit(int code)
{
    std::fflush(stdout);
    std::fflush(stderr);
    std::_Exit(code);
}

static void print_usage()
{
    std::fprintf(stderr,
                 "Usage: geometer <command> [options]\n"
                 "\n"
                 "Commands:\n"
                 "  --version                               Print version information\n"
                 "  model-bounds <input.step> <output.json> Emit source model bounds JSON\n"
                 "  model-to-glb <input.step> <output.glb>   Convert source model to GLB\n"
                 "  model-project-hlr <input.step> <output.json>\n"
                 "                                               Emit source model HLR JSON\n"
                 "  model-project-svg <input.step> <output.svg>\n"
                 "                                               Emit source model HLR SVG\n"
                 "  step-to-glb <input.step> <output.glb>       Convert STEP to GLB\n"
                 "  step-project-hlr <input.step> <output.json> Emit HLR projection JSON\n"
                 "  step-project-svg <input.step> <output.svg>  Emit HLR projection SVG\n"
                 "  planar-step <request.json> <output.step>    Emit exact planar STEP\n"
                 "  run <request.json> <response.json>          Run JSON batch jobs\n"
                 "  init-request <request.json> --step <path>   Write a starter JSON request\n"
                 "  planar-batch-solve <request.bin> <response.bin>\n"
                 "                                               Solve packed planar batch bytes\n"
#ifndef __EMSCRIPTEN__
                 "  serve --stdio                                Serve framed executable IPC A0\n"
#endif
                 "\n"
                 "Options:\n"
                 "  --deflection <value>   Absolute linear deflection (forces absolute mode)\n"
                 "  --deflection-mode <absolute|bbox-relative>  (default: bbox-relative)\n"
                 "  --deflection-coefficient <value>            (default: 0.004)\n"
                 "  --angular <value>      Angular deflection (default: 0.5)\n"
                 "  --projection-algorithm <poly|exact|fast>\n"
                 "  --outline-algorithm <hlr-close|mesh-shadow>\n"
                 "  --format <step>        Source model format (only step is supported)\n"
                 "  --view <id>            SVG view id (default: top)\n"
                 "  --mode <outline|detail|bbox> SVG mode (default: outline)\n"
                 "  --curve-mode <native-arcs|polyline>\n"
                 "  --samples <count>      Curve polyline samples (default: 24)\n"
                 "  --round-digits <count> Projection rounding digits (default: 3)\n"
                 "  --operation <name>     init-request operation\n"
                 "  --output <path>        init-request output path\n"
                 "  --repeat <count>       Planar benchmark repeat count (default: 1)\n"
                 "  --warmup <count>       Planar benchmark warmup count (default: 0)\n"
                 "  --metrics <path>       Write planar benchmark JSON metrics\n"
                 "  --format <binary|json> planar-batch-solve output format\n"
                 "  --return-rings <true|false> alias for --format json\n");
}

static bool read_file_bytes(const char* path, std::vector<unsigned char>* bytes)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return false;
    }
    bytes->assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return input.good() || input.eof();
}

static bool read_text_file(const char* path, std::string* text)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return false;
    }
    text->assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    const char utf8_bom[] = "\xEF\xBB\xBF";
    if (text->size() >= 3 && text->compare(0, 3, utf8_bom) == 0)
    {
        text->erase(0, 3);
    }
    return input.good() || input.eof();
}

static bool write_text_file(const char* path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        return false;
    }
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    return output.good();
}

static bool write_file_bytes(const char* path, const std::vector<unsigned char>& bytes)
{
    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        return false;
    }
    if (!bytes.empty())
    {
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
    }
    return output.good();
}

static void add_string(rapidjson::Value& object, const char* key, const std::string& value,
                       rapidjson::Document::AllocatorType& allocator)
{
    rapidjson::Value json_key;
    json_key.SetString(key, allocator);
    rapidjson::Value json_value;
    json_value.SetString(value.c_str(), static_cast<rapidjson::SizeType>(value.size()), allocator);
    object.AddMember(json_key, json_value, allocator);
}

static std::string compact_json(const rapidjson::Value& value)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

static bool member_string(const rapidjson::Value& object, const char* key, std::string* value)
{
    if (!object.IsObject())
    {
        return false;
    }
    auto it = object.FindMember(key);
    if (it == object.MemberEnd() || !it->value.IsString())
    {
        return false;
    }
    *value = it->value.GetString();
    return true;
}

static std::string job_id_for_index(const rapidjson::Value& job, std::size_t index)
{
    std::string id;
    if (member_string(job, "id", &id) && !id.empty())
    {
        return id;
    }
    return "job_" + std::to_string(index + 1);
}

static std::string default_output_for_step(const std::string& step_path,
                                           const std::string& operation)
{
    std::string name = step_path;
    const std::size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
    {
        name = name.substr(slash + 1);
    }
    const std::size_t dot = name.find_last_of('.');
    if (dot != std::string::npos)
    {
        name = name.substr(0, dot);
    }
    if (name.empty())
    {
        name = "model";
    }
    if (operation == "step_to_glb" || operation == "model_to_glb")
    {
        return name + ".glb";
    }
    if (operation == "step_hlr_projection_svg" || operation == "model_hlr_projection_svg")
    {
        return name + ".projection.svg";
    }
    if (operation == "model_bounds_json")
    {
        return name + ".bounds.json";
    }
    return name + ".projection.json";
}

static bool preset_projection_view(const std::string& id, geometer::ProjectionViewSpec* view)
{
    if (view == nullptr)
    {
        return false;
    }
    if (id == "top")
    {
        *view = {"top", {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};
        return true;
    }
    if (id == "bottom" || id == "bot")
    {
        *view = {"bottom", {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}};
        return true;
    }
    if (id == "front")
    {
        *view = {"front", {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
        return true;
    }
    if (id == "back")
    {
        *view = {"back", {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
        return true;
    }
    if (id == "right")
    {
        *view = {"right", {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
        return true;
    }
    if (id == "left")
    {
        *view = {"left", {-1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
        return true;
    }
    return false;
}

static bool has_projection_view(const geometer::HlrProjectionOptions& options,
                                const std::string& id)
{
    for (const geometer::ProjectionViewSpec& view : options.views)
    {
        if (view.id == id)
        {
            return true;
        }
    }
    return false;
}

static void ensure_projection_view(geometer::HlrProjectionOptions* options, const std::string& id)
{
    if (options == nullptr || id.empty() || has_projection_view(*options, id))
    {
        return;
    }

    geometer::ProjectionViewSpec view;
    if (!preset_projection_view(id, &view))
    {
        return;
    }

    if (options->views.empty())
    {
        options->views = {view};
    }
    else
    {
        options->views.push_back(view);
    }
}

static std::string normalize_operation(const std::string& operation)
{
    if (operation == "model-project-hlr" || operation == "model_project_hlr")
    {
        return "model_hlr_projection_json";
    }
    if (operation == "model-project-svg" || operation == "model_project_svg")
    {
        return "model_hlr_projection_svg";
    }
    if (operation == "model-to-glb" || operation == "model_to_glb")
    {
        return "model_to_glb";
    }
    if (operation == "model-bounds" || operation == "model_bounds" || operation == "step-bounds" ||
        operation == "step_bounds")
    {
        return "model_bounds_json";
    }
    if (operation == "step-project-hlr" || operation == "step_project_hlr")
    {
        return "step_hlr_projection_json";
    }
    if (operation == "step-project-svg" || operation == "step_project_svg")
    {
        return "step_hlr_projection_svg";
    }
    if (operation == "step-to-glb" || operation == "step_to_glb")
    {
        return "step_to_glb";
    }
    if (operation == "planar-step" || operation == "planar_step")
    {
        return "planar_step";
    }
    return operation;
}

static bool is_supported_batch_operation(const std::string& operation)
{
    return operation == "step_hlr_projection_json" || operation == "step_hlr_projection_svg" ||
           operation == "step_to_glb" || operation == "model_hlr_projection_json" ||
           operation == "model_hlr_projection_svg" || operation == "model_to_glb" ||
           operation == "model_bounds_json";
}

static const rapidjson::Value* options_value_for_object(const rapidjson::Value& object)
{
    auto it = object.FindMember("options");
    if (it == object.MemberEnd() || it->value.IsNull())
    {
        return nullptr;
    }
    return &it->value;
}

static std::string options_json_for_value(const rapidjson::Value* value)
{
    if (value == nullptr || value->IsNull())
    {
        return "{}";
    }
    return compact_json(*value);
}

static int parse_hlr_options_layer(const rapidjson::Value* options_value,
                                   geometer::HlrProjectionOptions* options,
                                   std::string* error_message)
{
    if (options_value == nullptr || options_value->IsNull())
    {
        return 0;
    }

    geometer::Status status;
    const std::string options_json = options_json_for_value(options_value);
    const int code =
        geometer::parse_hlr_projection_options_json(options_json.c_str(), options, &status);
    if (code != 0)
    {
        *error_message = status.message;
    }
    return code;
}

static int parse_glb_options_layer(const rapidjson::Value* options_value,
                                   geometer::StepToGlbOptions* options, std::string* error_message)
{
    if (options_value == nullptr || options_value->IsNull())
    {
        return 0;
    }

    geometer::Status status;
    const std::string options_json = options_json_for_value(options_value);
    const int code =
        geometer::parse_step_to_glb_options_json(options_json.c_str(), options, &status);
    if (code != 0)
    {
        *error_message = status.message;
    }
    return code;
}

static int parse_model_bounds_options_layer(const rapidjson::Value* options_value,
                                            geometer::ModelBoundsOptions* options,
                                            std::string* error_message)
{
    if (options_value == nullptr || options_value->IsNull())
    {
        return 0;
    }

    geometer::Status status;
    const std::string options_json = options_json_for_value(options_value);
    const int code =
        geometer::parse_model_bounds_options_json(options_json.c_str(), options, &status);
    if (code != 0)
    {
        *error_message = status.message;
    }
    return code;
}

static int validate_model_format_value(const rapidjson::Value* value, std::string* error_message)
{
    if (value == nullptr || value->IsNull())
    {
        return 0;
    }
    if (!value->IsString())
    {
        *error_message = "format must be a string";
        return 2;
    }
    const std::string format = value->GetString();
    if (format == "step" || format == "STEP")
    {
        return 0;
    }
    *error_message = "model operations currently support only format=\"step\"";
    return 2;
}

static int validate_model_format_layer(const rapidjson::Value* options_value,
                                       std::string* error_message)
{
    if (options_value == nullptr || options_value->IsNull())
    {
        return 0;
    }
    auto it = options_value->FindMember("format");
    if (it == options_value->MemberEnd())
    {
        it = options_value->FindMember("model_format");
    }
    return it == options_value->MemberEnd()
               ? 0
               : validate_model_format_value(&it->value, error_message);
}

static int validate_model_format_for_job(const rapidjson::Value& job,
                                         const rapidjson::Value* batch_options,
                                         std::string* error_message)
{
    int code = validate_model_format_layer(batch_options, error_message);
    if (code != 0)
    {
        return code;
    }
    code = validate_model_format_layer(options_value_for_object(job), error_message);
    if (code != 0)
    {
        return code;
    }
    auto it = job.FindMember("format");
    if (it == job.MemberEnd())
    {
        it = job.FindMember("model_format");
    }
    return it == job.MemberEnd() ? 0 : validate_model_format_value(&it->value, error_message);
}

static bool model_path_for_job(const rapidjson::Value& job, std::string* path)
{
    if (member_string(job, "model_path", path) && !path->empty())
    {
        return true;
    }
    return member_string(job, "step_path", path) && !path->empty();
}

static int parse_projection_options(int argc, char* argv[], int start,
                                    geometer::HlrProjectionOptions* options, std::string* view_id,
                                    std::string* mode)
{
    for (int i = start; i < argc - 1; i += 2)
    {
        if (std::strcmp(argv[i], "--view") == 0)
        {
            *view_id = argv[i + 1];
            ensure_projection_view(options, *view_id);
        }
        else if (std::strcmp(argv[i], "--mode") == 0)
        {
            *mode = argv[i + 1];
        }
        else if (std::strcmp(argv[i], "--curve-mode") == 0)
        {
            if (std::strcmp(argv[i + 1], "polyline") == 0)
            {
                options->curve_mode = geometer::ProjectionCurveMode::Polyline;
            }
            else
            {
                options->curve_mode = geometer::ProjectionCurveMode::NativeArcs;
            }
        }
        else if (std::strcmp(argv[i], "--samples") == 0)
        {
            options->samples_per_curve = std::atoi(argv[i + 1]);
        }
        else if (std::strcmp(argv[i], "--round-digits") == 0)
        {
            options->round_digits = std::atoi(argv[i + 1]);
        }
        else if (std::strcmp(argv[i], "--deflection") == 0)
        {
            options->mesh_linear_deflection = std::atof(argv[i + 1]);
            options->mesh_deflection_mode = geometer::MeshDeflectionMode::Absolute;
        }
        else if (std::strcmp(argv[i], "--angular") == 0)
        {
            options->mesh_angular_deflection = std::atof(argv[i + 1]);
        }
        else if (std::strcmp(argv[i], "--deflection-mode") == 0)
        {
            options->mesh_deflection_mode = std::strcmp(argv[i + 1], "absolute") == 0
                                                ? geometer::MeshDeflectionMode::Absolute
                                                : geometer::MeshDeflectionMode::BboxRelative;
        }
        else if (std::strcmp(argv[i], "--deflection-coefficient") == 0)
        {
            options->mesh_deflection_coefficient = std::atof(argv[i + 1]);
            options->mesh_deflection_mode = geometer::MeshDeflectionMode::BboxRelative;
        }
        else if (std::strcmp(argv[i], "--outline-algorithm") == 0)
        {
            const char* value = argv[i + 1];
            options->outline_algorithm =
                (std::strcmp(value, "mesh-shadow") == 0 || std::strcmp(value, "mesh_shadow") == 0)
                    ? geometer::ProjectionOutlineAlgorithm::MeshShadow
                    : geometer::ProjectionOutlineAlgorithm::HlrClosedEdges;
        }
        else if (std::strcmp(argv[i], "--projection-algorithm") == 0)
        {
            const char* value = argv[i + 1];
            if (std::strcmp(value, "fast") == 0)
            {
                options->projection_algorithm = geometer::ProjectionAlgorithm::Fast;
            }
            else if (std::strcmp(value, "exact") == 0)
            {
                options->projection_algorithm = geometer::ProjectionAlgorithm::Exact;
            }
            else if (std::strcmp(value, "poly") == 0)
            {
                options->projection_algorithm = geometer::ProjectionAlgorithm::Poly;
            }
            else
            {
                std::fprintf(stderr,
                             "--projection-algorithm must be poly, exact, or fast (got %s).\n",
                             value);
                return 2;
            }
        }
    }
    return 0;
}

static int validate_model_format_args(int argc, char* argv[], int start)
{
    for (int i = start; i < argc; i += 1)
    {
        if (std::strcmp(argv[i], "--format") != 0)
        {
            continue;
        }
        if (i + 1 >= argc)
        {
            std::fprintf(stderr, "--format requires a value.\n");
            return 2;
        }
        const char* format = argv[++i];
        if (std::strcmp(format, "step") != 0 && std::strcmp(format, "STEP") != 0)
        {
            std::fprintf(stderr, "Only --format step is currently supported.\n");
            return 2;
        }
    }
    return 0;
}

static int execute_hlr_job(const rapidjson::Value& job, const rapidjson::Value* batch_options,
                           const std::string& operation, std::string* output_path,
                           std::string* error_message)
{
    std::string step_path;
    if (!model_path_for_job(job, &step_path))
    {
        *error_message = "job requires string model_path or step_path";
        return 2;
    }
    if (!member_string(job, "output_path", output_path) || output_path->empty())
    {
        *error_message = "job requires string output_path";
        return 2;
    }

    std::vector<unsigned char> step_bytes;
    if (!read_file_bytes(step_path.c_str(), &step_bytes))
    {
        *error_message = "failed reading " + step_path;
        return 1;
    }

    geometer::HlrProjectionOptions options;
    int code = validate_model_format_for_job(job, batch_options, error_message);
    if (code != 0)
    {
        return code;
    }
    code = parse_hlr_options_layer(batch_options, &options, error_message);
    if (code != 0)
    {
        return code;
    }
    code = parse_hlr_options_layer(options_value_for_object(job), &options, error_message);
    if (code != 0)
    {
        return code;
    }

    geometer::HlrProjectionResult projection;
    geometer::Status status;
    code = geometer::step_hlr_projection_from_bytes(step_bytes.data(), step_bytes.size(), options,
                                                    &projection, &status);
    if (code != 0)
    {
        *error_message = status.message;
        return code;
    }

    std::string text;
    status = {};
    if (operation == "step_hlr_projection_svg" || operation == "model_hlr_projection_svg")
    {
        std::string view_id = "top";
        std::string mode = "outline";
        member_string(job, "view", &view_id);
        member_string(job, "view_id", &view_id);
        member_string(job, "mode", &mode);
        ensure_projection_view(&options, view_id);
        code = geometer::write_hlr_projection_svg(projection, view_id, mode, &text, &status);
    }
    else
    {
        code = geometer::write_hlr_projection_json(projection, &text, &status);
    }
    if (code != 0)
    {
        *error_message = status.message;
        return code;
    }
    if (!write_text_file(output_path->c_str(), text))
    {
        *error_message = "failed writing " + *output_path;
        return 1;
    }
    return 0;
}

static int execute_glb_job(const rapidjson::Value& job, const rapidjson::Value* batch_options,
                           std::string* output_path, std::string* error_message)
{
    std::string step_path;
    if (!model_path_for_job(job, &step_path))
    {
        *error_message = "job requires string model_path or step_path";
        return 2;
    }
    if (!member_string(job, "output_path", output_path) || output_path->empty())
    {
        *error_message = "job requires string output_path";
        return 2;
    }

    geometer::StepToGlbOptions options;
    int code = validate_model_format_for_job(job, batch_options, error_message);
    if (code != 0)
    {
        return code;
    }
    code = parse_glb_options_layer(batch_options, &options, error_message);
    if (code != 0)
    {
        return code;
    }
    code = parse_glb_options_layer(options_value_for_object(job), &options, error_message);
    if (code != 0)
    {
        return code;
    }

    code = geometer::step_to_glb(step_path, *output_path, options);
    if (code != 0)
    {
        *error_message = "STEP to GLB failed with code " + std::to_string(code);
        return code;
    }
    return 0;
}

static int execute_model_bounds_job(const rapidjson::Value& job,
                                    const rapidjson::Value* batch_options, std::string* output_path,
                                    std::string* error_message)
{
    std::string model_path;
    if (!model_path_for_job(job, &model_path))
    {
        *error_message = "job requires string model_path or step_path";
        return 2;
    }
    if (!member_string(job, "output_path", output_path) || output_path->empty())
    {
        *error_message = "job requires string output_path";
        return 2;
    }

    std::vector<unsigned char> model_bytes;
    if (!read_file_bytes(model_path.c_str(), &model_bytes))
    {
        *error_message = "failed reading " + model_path;
        return 1;
    }

    geometer::ModelBoundsOptions options;
    int code = parse_model_bounds_options_layer(batch_options, &options, error_message);
    if (code != 0)
    {
        return code;
    }
    code = parse_model_bounds_options_layer(options_value_for_object(job), &options, error_message);
    if (code != 0)
    {
        return code;
    }
    auto format_it = job.FindMember("format");
    if (format_it == job.MemberEnd())
    {
        format_it = job.FindMember("model_format");
    }
    if (format_it != job.MemberEnd())
    {
        rapidjson::Document format_options;
        format_options.SetObject();
        rapidjson::Document::AllocatorType& allocator = format_options.GetAllocator();
        rapidjson::Value key;
        key.SetString("format", allocator);
        rapidjson::Value value(format_it->value, allocator);
        format_options.AddMember(key, value, allocator);
        code = parse_model_bounds_options_layer(&format_options, &options, error_message);
        if (code != 0)
        {
            return code;
        }
    }

    geometer::ModelBoundsResult bounds;
    geometer::Status status;
    code = geometer::model_bounds_from_bytes(model_bytes.data(), model_bytes.size(), options,
                                             &bounds, &status);
    if (code != 0)
    {
        *error_message = status.message;
        return code;
    }

    std::string text;
    status = {};
    code = geometer::write_model_bounds_json(bounds, &text, &status);
    if (code != 0)
    {
        *error_message = status.message;
        return code;
    }
    if (!write_text_file(output_path->c_str(), text))
    {
        *error_message = "failed writing " + *output_path;
        return 1;
    }
    return 0;
}

static int execute_planar_step_job(const rapidjson::Value& job, std::string* output_path,
                                   std::string* error_message)
{
    if (!member_string(job, "output_path", output_path) || output_path->empty())
    {
        *error_message = "job requires string output_path";
        return 2;
    }

    std::string request_json;
    std::string request_path;
    if (member_string(job, "request_path", &request_path) && !request_path.empty())
    {
        if (!read_text_file(request_path.c_str(), &request_json))
        {
            *error_message = "failed reading " + request_path;
            return 1;
        }
    }
    else
    {
        const rapidjson::Value* inline_request = options_value_for_object(job);
        auto explicit_request = job.FindMember("planar_step_request");
        if (explicit_request != job.MemberEnd())
        {
            inline_request = &explicit_request->value;
        }
        if (inline_request == nullptr || !inline_request->IsObject())
        {
            *error_message = "job requires request_path or planar_step_request object";
            return 2;
        }
        request_json = compact_json(*inline_request);
    }

    geometer::PlanarStepResult summary;
    geometer::Status status;
    const int code =
        geometer::planar_step_from_json(request_json.c_str(), *output_path, &summary, &status);
    if (code != 0)
    {
        *error_message = status.message;
        return code;
    }
    return 0;
}

static int execute_batch_job(const rapidjson::Value& job, const rapidjson::Value* batch_options,
                             std::string* operation, std::string* output_path,
                             std::string* error_message)
{
    if (!member_string(job, "operation", operation) || operation->empty())
    {
        *error_message = "job requires string operation";
        return 2;
    }
    *operation = normalize_operation(*operation);
    if (*operation == "step_hlr_projection_json" || *operation == "step_hlr_projection_svg" ||
        *operation == "model_hlr_projection_json" || *operation == "model_hlr_projection_svg")
    {
        return execute_hlr_job(job, batch_options, *operation, output_path, error_message);
    }
    if (*operation == "step_to_glb" || *operation == "model_to_glb")
    {
        return execute_glb_job(job, batch_options, output_path, error_message);
    }
    if (*operation == "model_bounds_json")
    {
        return execute_model_bounds_job(job, batch_options, output_path, error_message);
    }
    if (*operation == "planar_step")
    {
        return execute_planar_step_job(job, output_path, error_message);
    }
    *error_message = "unsupported operation: " + *operation;
    return 2;
}

static void add_error_job(rapidjson::Value& jobs, rapidjson::Document::AllocatorType& allocator,
                          const std::string& id, const std::string& operation, int code,
                          const std::string& message)
{
    rapidjson::Value job(rapidjson::kObjectType);
    add_string(job, "id", id, allocator);
    add_string(job, "operation", operation, allocator);
    job.AddMember("ok", false, allocator);
    job.AddMember("code", code, allocator);
    add_string(job, "message", message, allocator);
    jobs.PushBack(job, allocator);
}

static int run_batch_request(const char* request_path, const char* response_path)
{
    std::string request_text;
    rapidjson::Document response;
    response.SetObject();
    rapidjson::Document::AllocatorType& allocator = response.GetAllocator();
    add_string(response, "schema", "geometer.batch.response.a0", allocator);
    add_string(response, "version", geometer::version_string(), allocator);
    response.AddMember("abi", geometer::abi_version(), allocator);
    rapidjson::Value response_jobs(rapidjson::kArrayType);

    int return_code = 0;
    if (!read_text_file(request_path, &request_text))
    {
        response.AddMember("ok", false, allocator);
        add_error_job(response_jobs, allocator, "request", "run", 1,
                      std::string("failed reading ") + request_path);
        response.AddMember("jobs", response_jobs, allocator);
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        write_text_file(response_path, buffer.GetString());
        return 1;
    }

    rapidjson::Document request;
    request.Parse(request_text.c_str());
    if (request.HasParseError() || !request.IsObject())
    {
        response.AddMember("ok", false, allocator);
        std::string message = "invalid request JSON";
        if (request.HasParseError())
        {
            message += ": ";
            message += rapidjson::GetParseError_En(request.GetParseError());
        }
        add_error_job(response_jobs, allocator, "request", "run", 2, message);
        response.AddMember("jobs", response_jobs, allocator);
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        write_text_file(response_path, buffer.GetString());
        return 2;
    }

    auto jobs_it = request.FindMember("jobs");
    if (jobs_it == request.MemberEnd() || !jobs_it->value.IsArray())
    {
        response.AddMember("ok", false, allocator);
        add_error_job(response_jobs, allocator, "request", "run", 2, "request requires jobs array");
        response.AddMember("jobs", response_jobs, allocator);
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        write_text_file(response_path, buffer.GetString());
        return 2;
    }

    const rapidjson::Value* batch_options = nullptr;
    auto options_it = request.FindMember("options");
    if (options_it != request.MemberEnd())
    {
        if (!options_it->value.IsNull() && !options_it->value.IsObject())
        {
            response.AddMember("ok", false, allocator);
            add_error_job(response_jobs, allocator, "request", "run", 2,
                          "request options must be an object or null");
            response.AddMember("jobs", response_jobs, allocator);
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            response.Accept(writer);
            write_text_file(response_path, buffer.GetString());
            return 2;
        }
        batch_options = &options_it->value;
    }

    std::size_t index = 0;
    for (const rapidjson::Value& request_job : jobs_it->value.GetArray())
    {
        rapidjson::Value response_job(rapidjson::kObjectType);
        const std::string id = job_id_for_index(request_job, index);
        add_string(response_job, "id", id, allocator);

        const auto started = std::chrono::steady_clock::now();
        std::string operation;
        std::string output_path;
        std::string error_message;
        int code = 2;
        if (!request_job.IsObject())
        {
            error_message = "job must be an object";
        }
        else
        {
            code = execute_batch_job(request_job, batch_options, &operation, &output_path,
                                     &error_message);
        }
        const auto finished = std::chrono::steady_clock::now();
        const double elapsed_ms =
            std::chrono::duration<double, std::milli>(finished - started).count();

        add_string(response_job, "operation", operation.empty() ? "unknown" : operation, allocator);
        response_job.AddMember("ok", code == 0, allocator);
        response_job.AddMember("code", code, allocator);
        response_job.AddMember("elapsed_ms", elapsed_ms, allocator);
        if (!output_path.empty())
        {
            add_string(response_job, "output_path", output_path, allocator);
        }
        if (code != 0)
        {
            add_string(response_job, "message", error_message, allocator);
            return_code = 1;
        }
        response_jobs.PushBack(response_job, allocator);
        index += 1;
    }

    response.AddMember("ok", return_code == 0, allocator);
    response.AddMember("jobs", response_jobs, allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    response.Accept(writer);
    if (!write_text_file(response_path, buffer.GetString()))
    {
        std::fprintf(stderr, "Failed writing %s\n", response_path);
        return 1;
    }
    return return_code;
}

static int init_request(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::fprintf(stderr, "init-request requires output request path and --step <path>.\n");
        return 1;
    }
    const std::string request_path = argv[2];
    std::string step_path;
    std::string operation = "step_hlr_projection_json";
    std::string output_path;

    for (int i = 3; i < argc; i += 1)
    {
        if (std::strcmp(argv[i], "--step") == 0 && i + 1 < argc)
        {
            step_path = argv[++i];
        }
        else if (std::strcmp(argv[i], "--operation") == 0 && i + 1 < argc)
        {
            operation = normalize_operation(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc)
        {
            output_path = argv[++i];
        }
    }

    if (step_path.empty())
    {
        std::fprintf(stderr, "init-request requires --step <path>.\n");
        return 1;
    }
    if (output_path.empty())
    {
        output_path = default_output_for_step(step_path, operation);
    }
    if (!is_supported_batch_operation(operation))
    {
        std::fprintf(stderr, "Unsupported operation for init-request: %s\n", operation.c_str());
        return 1;
    }

    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    add_string(document, "schema", "geometer.batch.request.a0", allocator);
    add_string(document, "version", geometer::version_string(), allocator);
    document.AddMember("abi", geometer::abi_version(), allocator);

    rapidjson::Value jobs(rapidjson::kArrayType);
    rapidjson::Value job(rapidjson::kObjectType);
    add_string(job, "id", "job_1", allocator);
    add_string(job, "operation", operation, allocator);
    add_string(job, "step_path", step_path, allocator);
    add_string(job, "output_path", output_path, allocator);

    rapidjson::Value options(rapidjson::kObjectType);
    if (operation == "step_to_glb" || operation == "model_to_glb")
    {
        options.AddMember("linear_deflection", 0.1, allocator);
        options.AddMember("angular_deflection", 0.5, allocator);
    }
    else if (operation == "model_bounds_json")
    {
        add_string(options, "format", "step", allocator);
    }
    else
    {
        rapidjson::Value views(rapidjson::kArrayType);
        rapidjson::Value view(rapidjson::kObjectType);
        add_string(view, "id", "top", allocator);
        rapidjson::Value direction(rapidjson::kArrayType);
        direction.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(1.0, allocator);
        rapidjson::Value up(rapidjson::kArrayType);
        up.PushBack(0.0, allocator).PushBack(1.0, allocator).PushBack(0.0, allocator);
        view.AddMember("direction", direction, allocator);
        view.AddMember("up", up, allocator);
        views.PushBack(view, allocator);
        options.AddMember("views", views, allocator);
        add_string(options, "curve_mode", "polyline", allocator);
        options.AddMember("samples_per_curve", 24, allocator);
        options.AddMember("round_digits", 3, allocator);
        options.AddMember("include_visible", true, allocator);
        options.AddMember("include_outline", true, allocator);
        options.AddMember("union_outline_polygons", true, allocator);
    }
    job.AddMember("options", options, allocator);
    jobs.PushBack(job, allocator);
    document.AddMember("jobs", jobs, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    if (!write_text_file(request_path.c_str(), buffer.GetString()))
    {
        std::fprintf(stderr, "Failed writing %s\n", request_path.c_str());
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2 || std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "help") == 0 ||
        std::strcmp(argv[1], "-h") == 0)
    {
        print_usage();
        return argc < 2 ? 1 : 0;
    }

    if (std::strcmp(argv[1], "--version") == 0 || std::strcmp(argv[1], "version") == 0)
    {
        std::printf("geometer %s (abi %d)\n", geometer::version_string(), geometer::abi_version());
        clean_exit(0);
    }

#ifndef __EMSCRIPTEN__
    if (std::strcmp(argv[1], "serve") == 0)
    {
        if (argc != 3 || std::strcmp(argv[2], "--stdio") != 0)
        {
            std::fprintf(stderr, "serve requires exactly --stdio.\n");
            return 1;
        }
        clean_exit(geometer::ipc_a0::serve_stdio());
    }
#endif

    if (std::strcmp(argv[1], "run") == 0)
    {
        if (argc < 4)
        {
            std::fprintf(stderr, "run requires request and response JSON paths.\n");
            return 1;
        }
        const int result = run_batch_request(argv[2], argv[3]);
        clean_exit(result);
    }

    if (std::strcmp(argv[1], "init-request") == 0)
    {
        const int result = init_request(argc, argv);
        clean_exit(result);
    }

    if (std::strcmp(argv[1], "step-to-glb") == 0 || std::strcmp(argv[1], "model-to-glb") == 0)
    {
        if (argc < 4)
        {
            std::fprintf(stderr, "%s requires input and output paths.\n", argv[1]);
            return 1;
        }

        const char* input = argv[2];
        const char* output = argv[3];

        geometer::StepToGlbOptions options;
        const int format_result = validate_model_format_args(argc, argv, 4);
        if (format_result != 0)
        {
            return format_result;
        }

        for (int i = 4; i < argc - 1; i += 2)
        {
            if (std::strcmp(argv[i], "--deflection") == 0)
            {
                options.linear_deflection = std::atof(argv[i + 1]);
            }
            else if (std::strcmp(argv[i], "--angular") == 0)
            {
                options.angular_deflection = std::atof(argv[i + 1]);
            }
        }

        std::fprintf(stderr, "Converting %s -> %s\n", input, output);
        int result = geometer::step_to_glb(input, output, options);

        if (result == 0)
        {
            std::fprintf(stderr, "Done.\n");
        }
        else
        {
            std::fprintf(stderr, "Failed (error %d).\n", result);
        }
        clean_exit(result);
    }

    if (std::strcmp(argv[1], "model-bounds") == 0)
    {
        if (argc < 4)
        {
            std::fprintf(stderr, "model-bounds requires input and output paths.\n");
            return 1;
        }

        const int format_result = validate_model_format_args(argc, argv, 4);
        if (format_result != 0)
        {
            return format_result;
        }

        std::vector<unsigned char> model_bytes;
        if (!read_file_bytes(argv[2], &model_bytes))
        {
            std::fprintf(stderr, "Failed reading %s\n", argv[2]);
            return 1;
        }

        geometer::ModelBoundsOptions options;
        geometer::ModelBoundsResult bounds;
        geometer::Status status;
        int result = geometer::model_bounds_from_bytes(model_bytes.data(), model_bytes.size(),
                                                       options, &bounds, &status);
        if (result != 0)
        {
            std::fprintf(stderr, "Model bounds failed (%d): %s\n", result, status.message.c_str());
            return result;
        }

        std::string text;
        result = geometer::write_model_bounds_json(bounds, &text, &status);
        if (result != 0)
        {
            std::fprintf(stderr, "Model bounds output failed (%d): %s\n", result,
                         status.message.c_str());
            return result;
        }
        if (!write_text_file(argv[3], text))
        {
            std::fprintf(stderr, "Failed writing %s\n", argv[3]);
            return 1;
        }
        clean_exit(0);
    }

    if (std::strcmp(argv[1], "planar-step") == 0)
    {
        if (argc < 4)
        {
            std::fprintf(stderr, "planar-step requires request JSON and output STEP paths.\n");
            return 1;
        }

        std::string request_json;
        if (!read_text_file(argv[2], &request_json))
        {
            std::fprintf(stderr, "Failed reading %s\n", argv[2]);
            return 1;
        }

        geometer::PlanarStepResult summary;
        geometer::Status status;
        const int result =
            geometer::planar_step_from_json(request_json.c_str(), argv[3], &summary, &status);
        if (result != 0)
        {
            std::fprintf(stderr, "Planar STEP failed (%d): %s\n", result, status.message.c_str());
            return result;
        }
        std::fprintf(stderr, "Planar STEP: %d bodies, %d regions, %d cutouts -> %s\n",
                     summary.body_count, summary.region_count, summary.cutout_count, argv[3]);
        clean_exit(0);
    }

    if (std::strcmp(argv[1], "step-project-hlr") == 0 ||
        std::strcmp(argv[1], "step-project-svg") == 0 ||
        std::strcmp(argv[1], "model-project-hlr") == 0 ||
        std::strcmp(argv[1], "model-project-svg") == 0)
    {
        if (argc < 4)
        {
            std::fprintf(stderr, "%s requires input and output paths.\n", argv[1]);
            return 1;
        }

        const char* input = argv[2];
        const char* output = argv[3];
        geometer::HlrProjectionOptions options;
        std::string view_id = "top";
        std::string mode = "outline";
        const int format_result = validate_model_format_args(argc, argv, 4);
        if (format_result != 0)
        {
            return format_result;
        }
        const int projection_options_result =
            parse_projection_options(argc, argv, 4, &options, &view_id, &mode);
        if (projection_options_result != 0)
        {
            return projection_options_result;
        }

        std::vector<unsigned char> step_bytes;
        if (!read_file_bytes(input, &step_bytes))
        {
            std::fprintf(stderr, "Failed reading %s\n", input);
            return 1;
        }

        geometer::HlrProjectionResult projection;
        geometer::Status status;
        int result = geometer::step_hlr_projection_from_bytes(step_bytes.data(), step_bytes.size(),
                                                              options, &projection, &status);
        if (result != 0)
        {
            std::fprintf(stderr, "Projection failed (%d): %s\n", result, status.message.c_str());
            return result;
        }

        std::string text;
        if (std::strcmp(argv[1], "step-project-svg") == 0 ||
            std::strcmp(argv[1], "model-project-svg") == 0)
        {
            result = geometer::write_hlr_projection_svg(projection, view_id, mode, &text, &status);
        }
        else
        {
            result = geometer::write_hlr_projection_json(projection, &text, &status);
        }
        if (result != 0)
        {
            std::fprintf(stderr, "Output failed (%d): %s\n", result, status.message.c_str());
            return result;
        }
        if (!write_text_file(output, text))
        {
            std::fprintf(stderr, "Failed writing %s\n", output);
            return 1;
        }

        clean_exit(0);
    }

    if (std::strcmp(argv[1], "planar-batch-solve") == 0)
    {
        if (argc < 4)
        {
            std::fprintf(stderr, "planar-batch-solve requires request and response paths.\n");
            return 1;
        }

        const char* input = argv[2];
        const char* output = argv[3];
        int repeat = 1;
        int warmup = 0;
        const char* metrics_path = nullptr;
        bool output_json = false;
        for (int i = 4; i < argc; i += 1)
        {
            if (std::strcmp(argv[i], "--repeat") == 0 && i + 1 < argc)
            {
                repeat = std::max(1, std::atoi(argv[++i]));
            }
            else if (std::strcmp(argv[i], "--warmup") == 0 && i + 1 < argc)
            {
                warmup = std::max(0, std::atoi(argv[++i]));
            }
            else if (std::strcmp(argv[i], "--metrics") == 0 && i + 1 < argc)
            {
                metrics_path = argv[++i];
            }
            else if (std::strcmp(argv[i], "--format") == 0 && i + 1 < argc)
            {
                const char* value = argv[++i];
                output_json = std::strcmp(value, "json") == 0;
            }
            else if (std::strcmp(argv[i], "--return-rings") == 0 && i + 1 < argc)
            {
                const char* value = argv[++i];
                output_json = std::strcmp(value, "0") != 0 && std::strcmp(value, "false") != 0;
            }
        }

        std::vector<unsigned char> request_bytes;
        if (!read_file_bytes(input, &request_bytes))
        {
            std::fprintf(stderr, "Failed reading %s\n", input);
            return 1;
        }

        std::vector<unsigned char> response_bytes;
        std::string response_json;
        geometer::Status status;
        for (int i = 0; i < warmup; i += 1)
        {
            status = {};
            int code = 0;
            if (output_json)
            {
                std::string warmup_response;
                code = geometer::solve_planar_batch_json_from_bytes(
                    request_bytes.data(), request_bytes.size(), &warmup_response, &status);
            }
            else
            {
                std::vector<unsigned char> warmup_response;
                code = geometer::solve_planar_batch_from_bytes(
                    request_bytes.data(), request_bytes.size(), &warmup_response, &status);
            }
            if (code != 0)
            {
                std::fprintf(stderr, "Planar warmup failed (%d): %s\n", code,
                             status.message.c_str());
                return code;
            }
        }

        std::vector<double> timings_ms;
        timings_ms.reserve(static_cast<std::size_t>(repeat));
        for (int i = 0; i < repeat; i += 1)
        {
            status = {};
            const auto started = std::chrono::steady_clock::now();
            int code = 0;
            if (output_json)
            {
                std::string run_response;
                code = geometer::solve_planar_batch_json_from_bytes(
                    request_bytes.data(), request_bytes.size(), &run_response, &status);
                response_json = std::move(run_response);
            }
            else
            {
                std::vector<unsigned char> run_response;
                code = geometer::solve_planar_batch_from_bytes(
                    request_bytes.data(), request_bytes.size(), &run_response, &status);
                response_bytes = std::move(run_response);
            }
            const auto finished = std::chrono::steady_clock::now();
            if (code != 0)
            {
                std::fprintf(stderr, "Planar solve failed (%d): %s\n", code,
                             status.message.c_str());
                return code;
            }
            const auto elapsed =
                std::chrono::duration<double, std::milli>(finished - started).count();
            timings_ms.push_back(elapsed);
        }

        const bool wrote_output = output_json ? write_text_file(output, response_json)
                                              : write_file_bytes(output, response_bytes);
        if (!wrote_output)
        {
            std::fprintf(stderr, "Failed writing %s\n", output);
            return 1;
        }

        const std::size_t response_size =
            output_json ? response_json.size() : response_bytes.size();
        const double min_ms = *std::min_element(timings_ms.begin(), timings_ms.end());
        const double max_ms = *std::max_element(timings_ms.begin(), timings_ms.end());
        const double mean_ms = std::accumulate(timings_ms.begin(), timings_ms.end(), 0.0) /
                               static_cast<double>(timings_ms.size());
        const double last_ms = timings_ms.empty() ? 0.0 : timings_ms.back();
        char metrics[1024];
        std::snprintf(metrics, sizeof(metrics),
                      "{\n"
                      "  \"version\": \"%s\",\n"
                      "  \"abi\": %d,\n"
                      "  \"requestBytes\": %zu,\n"
                      "  \"responseBytes\": %zu,\n"
                      "  \"format\": \"%s\",\n"
                      "  \"warmup\": %d,\n"
                      "  \"repeat\": %d,\n"
                      "  \"minMs\": %.6f,\n"
                      "  \"meanMs\": %.6f,\n"
                      "  \"maxMs\": %.6f,\n"
                      "  \"lastMs\": %.6f\n"
                      "}\n",
                      geometer::version_string(), geometer::abi_version(), request_bytes.size(),
                      response_size, output_json ? "json" : "binary", warmup, repeat, min_ms,
                      mean_ms, max_ms, last_ms);
        std::printf("%s", metrics);
        if (metrics_path != nullptr && !write_text_file(metrics_path, metrics))
        {
            std::fprintf(stderr, "Failed writing %s\n", metrics_path);
            return 1;
        }

        clean_exit(0);
    }

    std::fprintf(stderr, "Unknown command: %s\n", argv[1]);
    print_usage();
    return 1;
}
