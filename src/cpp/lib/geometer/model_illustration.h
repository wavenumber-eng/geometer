#pragma once

#include "geometer/generated/contracts/contracts.h"
#include "geometer/status.h"

#include <cstddef>
#include <string>
#include <vector>

namespace geometer
{
struct ModelIllustrationAttachmentView
{
    const unsigned char* data = nullptr;
    std::size_t size = 0;
    std::string media_type = "application/step";
};

struct ModelIllustrationGeometry
{
    struct Metadata
    {
        contracts::ModelIllustrationBounds3MmA0 bounds_mm;
        contracts::ModelIllustrationSourceSummaryA0 source;
        contracts::MeshIllustrationRenderStats stats;
        contracts::ModelIllustrationTimingsA0 timings;
        std::vector<std::string> warnings;
    } metadata;
    contracts::MeshIllustrationGeometryA0 geometry;
};

/// Import or lower one model source and produce a complete SVG illustration.
/// A model source requires one STEP attachment; an analytic source requires null.
/// STEP tessellation, Fast detail, Fast Mesh Shadow, visibility and fusion execute
/// in one native call without serializing an intermediate mesh collection.
int illustrate_model(const contracts::ModelIllustrationRequestA0& request,
                     const ModelIllustrationAttachmentView* model,
                     contracts::ModelIllustrationResultA0* result, Status* status = nullptr);

/// Run the same one-pass pipeline and return owning renderer-neutral geometry.
int illustrate_model_geometry(const contracts::ModelIllustrationGeometryRequestA0& request,
                              const ModelIllustrationAttachmentView* model,
                              ModelIllustrationGeometry* result, Status* status = nullptr);
} // namespace geometer
