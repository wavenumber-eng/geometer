#pragma once

#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>

namespace geometer
{

// Remove only the placement attached to each free-shape root. Assembly-child
// placements remain part of the definition geometry.
void strip_free_shape_root_locations(const Handle(TDocStd_Document) & document);

// Return all free shapes as one projection-ready compound.
TopoDS_Shape free_shape_compound(const Handle(TDocStd_Document) & document);

} // namespace geometer
