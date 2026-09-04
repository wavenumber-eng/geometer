#include "fast_hlr_occt.h"
#include "fast_hlr_prepare_internal.h"

#include <BRep_Tool.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <vector>

namespace geometer
{
namespace
{

struct VertexKey
{
    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t z = 0;
    std::uint32_t component = 0;

    bool operator==(const VertexKey& other) const
    {
        return x == other.x && y == other.y && z == other.z && component == other.component;
    }
};

struct VertexKeyHash
{
    std::size_t operator()(const VertexKey& key) const
    {
        std::size_t value = static_cast<std::size_t>(key.x);
        value ^= static_cast<std::size_t>(key.y) + 0x9e3779b9U + (value << 6U) + (value >> 2U);
        value ^= static_cast<std::size_t>(key.z) + 0x9e3779b9U + (value << 6U) + (value >> 2U);
        value ^=
            static_cast<std::size_t>(key.component) + 0x9e3779b9U + (value << 6U) + (value >> 2U);
        return value;
    }
};

void set_status(Status* status, int code, const char* message)
{
    if (status != nullptr)
    {
        status->code = code;
        status->message = message == nullptr ? "" : message;
    }
}

bool vertex_key(const gp_Pnt& point, double tolerance, std::uint32_t component, VertexKey* key)
{
    const double limit = static_cast<double>(std::numeric_limits<std::int64_t>::max() - 2);
    const double x = std::floor(point.X() / tolerance);
    const double y = std::floor(point.Y() / tolerance);
    const double z = std::floor(point.Z() / tolerance);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || std::fabs(x) > limit ||
        std::fabs(y) > limit || std::fabs(z) > limit)
    {
        return false;
    }
    *key = {static_cast<std::int64_t>(x), static_cast<std::int64_t>(y),
            static_cast<std::int64_t>(z), component};
    return true;
}

bool within_weld_tolerance(const gp_Pnt& first, const FastHlrVec3& second, double tolerance)
{
    return std::hypot(first.X() - second.x, first.Y() - second.y, first.Z() - second.z) <=
           tolerance;
}

std::uint32_t component_root(std::vector<std::uint32_t>* parents, std::uint32_t item)
{
    std::uint32_t root = item;
    while ((*parents)[root] != root)
    {
        root = (*parents)[root];
    }
    while ((*parents)[item] != item)
    {
        const std::uint32_t next = (*parents)[item];
        (*parents)[item] = root;
        item = next;
    }
    return root;
}

void join_components(std::vector<std::uint32_t>* parents, std::uint32_t first, std::uint32_t second)
{
    first = component_root(parents, first);
    second = component_root(parents, second);
    if (first != second)
    {
        (*parents)[second] = first;
    }
}

} // namespace

int prepare_fast_hlr_shape(const TopoDS_Shape& shape, const FastHlrOptions& options,
                           FastHlrPreparedMesh* prepared, Status* status)
{
    if (shape.IsNull())
    {
        set_status(status, 1, "Fast HLR source shape is null.");
        return 1;
    }
    if (prepared == nullptr)
    {
        set_status(status, 2, "Fast HLR prepared-mesh pointer is null.");
        return 2;
    }
    if (!std::isfinite(options.weld_tolerance) || options.weld_tolerance <= 0.0)
    {
        set_status(status, 3, "Fast HLR weld_tolerance must be finite and positive.");
        return 3;
    }

    FastHlrIndexedMesh indexed;
    std::unordered_map<VertexKey, std::vector<std::uint32_t>, VertexKeyHash> vertices;
    TopTools_IndexedMapOfShape faces;
    TopExp::MapShapes(shape, TopAbs_FACE, faces);
    std::vector<std::uint32_t> component_parents(static_cast<std::size_t>(faces.Extent()));
    std::iota(component_parents.begin(), component_parents.end(), 0U);
    TopTools_IndexedDataMapOfShapeListOfShape edge_faces;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edge_faces);
    for (int edge_index = 1; edge_index <= edge_faces.Extent(); ++edge_index)
    {
        std::uint32_t first_face = std::numeric_limits<std::uint32_t>::max();
        for (TopTools_ListOfShape::Iterator iterator(edge_faces.FindFromIndex(edge_index));
             iterator.More(); iterator.Next())
        {
            const int mapped = faces.FindIndex(iterator.Value());
            if (mapped <= 0)
            {
                continue;
            }
            const auto face = static_cast<std::uint32_t>(mapped - 1);
            if (first_face == std::numeric_limits<std::uint32_t>::max())
            {
                first_face = face;
            }
            else
            {
                join_components(&component_parents, first_face, face);
            }
        }
    }
    for (std::uint32_t& component : component_parents)
    {
        component = component_root(&component_parents, component);
    }

    for (int face_index = 1; face_index <= faces.Extent(); ++face_index)
    {
        const TopoDS_Face& face = TopoDS::Face(faces.FindKey(face_index));
        const auto source_face = static_cast<std::uint32_t>(face_index - 1);
        const std::uint32_t component = component_parents[source_face];
        TopLoc_Location location;
        const Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
        if (triangulation.IsNull())
        {
            continue;
        }
        const gp_Trsf transform = location.Transformation();
        std::vector<std::uint32_t> local_vertices(
            static_cast<std::size_t>(triangulation->NbNodes()) + 1);
        for (int node = 1; node <= triangulation->NbNodes(); ++node)
        {
            const gp_Pnt point = triangulation->Node(node).Transformed(transform);
            VertexKey key;
            if (!vertex_key(point, options.weld_tolerance, component, &key))
            {
                set_status(status, 4, "Fast HLR vertex exceeds the weld-grid range.");
                return 4;
            }
            std::uint32_t vertex = std::numeric_limits<std::uint32_t>::max();
            for (std::int64_t dz = -1; dz <= 1; ++dz)
                for (std::int64_t dy = -1; dy <= 1; ++dy)
                    for (std::int64_t dx = -1; dx <= 1; ++dx)
                    {
                        const auto found =
                            vertices.find({key.x + dx, key.y + dy, key.z + dz, key.component});
                        if (found == vertices.end())
                            continue;
                        for (std::uint32_t candidate : found->second)
                            if (candidate < vertex &&
                                within_weld_tolerance(point, indexed.vertices[candidate],
                                                      options.weld_tolerance))
                                vertex = candidate;
                    }
            if (vertex == std::numeric_limits<std::uint32_t>::max())
            {
                if (indexed.vertices.size() >= options.limits.max_vertices)
                {
                    set_status(status, 5, "Fast HLR shape exceeds the configured vertex limit.");
                    return 5;
                }
                vertex = static_cast<std::uint32_t>(indexed.vertices.size());
                indexed.vertices.push_back({point.X(), point.Y(), point.Z()});
                vertices[key].push_back(vertex);
            }
            local_vertices[static_cast<std::size_t>(node)] = vertex;
        }
        for (int triangle = 1; triangle <= triangulation->NbTriangles(); ++triangle)
        {
            int first = 0;
            int second = 0;
            int third = 0;
            triangulation->Triangle(triangle).Get(first, second, third);
            if (face.Orientation() == TopAbs_REVERSED)
            {
                std::swap(second, third);
            }
            if (indexed.triangles.size() >= options.limits.max_triangles)
            {
                set_status(status, 5, "Fast HLR shape exceeds the configured triangle limit.");
                return 5;
            }
            indexed.triangles.push_back({{local_vertices[static_cast<std::size_t>(first)],
                                          local_vertices[static_cast<std::size_t>(second)],
                                          local_vertices[static_cast<std::size_t>(third)]},
                                         source_face});
        }
    }
    return fast_hlr_internal::prepare_indexed_mesh_preserving_vertices(indexed, options, prepared,
                                                                       status);
}

} // namespace geometer
