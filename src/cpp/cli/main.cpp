#include "geometer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
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
    std::fprintf(stderr, "Usage: geometer <command> [options]\n"
                         "\n"
                         "Commands:\n"
                         "  --version                               Print version information\n"
                         "  step-to-glb <input.step> <output.glb>       Convert STEP to GLB\n"
                         "  step-project-hlr <input.step> <output.json> Emit HLR projection JSON\n"
                         "  step-project-svg <input.step> <output.svg>  Emit HLR projection SVG\n"
                         "\n"
                         "Options:\n"
                         "  --deflection <value>   Linear deflection (default: 0.1)\n"
                         "  --angular <value>      Angular deflection (default: 0.5)\n"
                         "  --view <id>            SVG view id (default: top)\n"
                         "  --mode <simple|detail> SVG mode (default: simple)\n"
                         "  --curve-mode <native-arcs|polyline>\n"
                         "  --samples <count>      Curve polyline samples (default: 24)\n"
                         "  --round-digits <count> Projection rounding digits (default: 3)\n");
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

static void parse_projection_options(int argc, char* argv[], int start,
                                     geometer::HlrProjectionOptions* options, std::string* view_id,
                                     std::string* mode)
{
    for (int i = start; i < argc - 1; i += 2)
    {
        if (std::strcmp(argv[i], "--view") == 0)
        {
            *view_id = argv[i + 1];
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
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    if (std::strcmp(argv[1], "--version") == 0 || std::strcmp(argv[1], "version") == 0)
    {
        std::printf("geometer %s (abi %d)\n", geometer::version_string(), geometer::abi_version());
        clean_exit(0);
    }

    if (std::strcmp(argv[1], "step-to-glb") == 0)
    {
        if (argc < 4)
        {
            std::fprintf(stderr, "step-to-glb requires input and output paths.\n");
            return 1;
        }

        const char* input = argv[2];
        const char* output = argv[3];

        geometer::StepToGlbOptions options;

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

    if (std::strcmp(argv[1], "step-project-hlr") == 0 ||
        std::strcmp(argv[1], "step-project-svg") == 0)
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
        std::string mode = "simple";
        parse_projection_options(argc, argv, 4, &options, &view_id, &mode);

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
        if (std::strcmp(argv[1], "step-project-svg") == 0)
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

    std::fprintf(stderr, "Unknown command: %s\n", argv[1]);
    print_usage();
    return 1;
}
