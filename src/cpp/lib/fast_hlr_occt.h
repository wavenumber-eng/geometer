#pragma once

#include "geometer/fast_hlr.h"

#include <TopoDS_Shape.hxx>

namespace geometer
{

int prepare_fast_hlr_shape(const TopoDS_Shape& shape, const FastHlrOptions& options,
                           FastHlrPreparedMesh* prepared, Status* status = nullptr);

} // namespace geometer
