#include "geometer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

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
        "  step-to-glb <input.step> <output.glb>  Convert STEP to GLB\n"
        "\n"
        "Options:\n"
        "  --deflection <value>   Linear deflection (default: 0.1)\n"
        "  --angular <value>      Angular deflection (default: 0.5)\n"
    );
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_usage();
        return 1;
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

    std::fprintf(stderr, "Unknown command: %s\n", argv[1]);
    print_usage();
    return 1;
}
