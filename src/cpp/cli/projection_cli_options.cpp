#include "projection_cli_options.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
bool preset_projection_view(const std::string& id, geometer::ProjectionViewSpec* view)
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

bool has_projection_view(const geometer::HlrProjectionOptions& options, const std::string& id)
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
} // namespace

void ensure_projection_view(geometer::HlrProjectionOptions* options, const std::string& id)
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

int parse_projection_options(int argc, char* argv[], int start,
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
            options->curve_mode = std::strcmp(argv[i + 1], "polyline") == 0
                                      ? geometer::ProjectionCurveMode::Polyline
                                      : geometer::ProjectionCurveMode::NativeArcs;
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
            if (std::strcmp(value, "fast-mesh-shadow") == 0 ||
                std::strcmp(value, "fast_mesh_shadow") == 0)
            {
                options->outline_algorithm = geometer::ProjectionOutlineAlgorithm::FastMeshShadow;
            }
            else if (std::strcmp(value, "mesh-shadow") == 0 ||
                     std::strcmp(value, "mesh_shadow") == 0)
            {
                options->outline_algorithm = geometer::ProjectionOutlineAlgorithm::MeshShadow;
            }
            else if (std::strcmp(value, "hlr-close") == 0 || std::strcmp(value, "hlr_close") == 0 ||
                     std::strcmp(value, "hlr") == 0)
            {
                options->outline_algorithm = geometer::ProjectionOutlineAlgorithm::HlrClosedEdges;
            }
            else
            {
                std::fprintf(stderr,
                             "--outline-algorithm must be hlr-close, mesh-shadow, or "
                             "fast-mesh-shadow (got %s).\n",
                             value);
                return 2;
            }
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
