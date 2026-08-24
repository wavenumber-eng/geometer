#include "step_topology_session_internal.h"

#include "geometer/sha256.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <IMeshTools_Parameters.hxx>
#include <Message_ProgressIndicator.hxx>
#include <Message_ProgressScope.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace geometer::step_topology_internal
{
namespace
{

class RenderCancellationProgress final : public Message_ProgressIndicator
{
    DEFINE_STANDARD_RTTIEXT(RenderCancellationProgress, Message_ProgressIndicator)

  public:
    explicit RenderCancellationProgress(const StepTopologyCancellation* cancellation)
        : cancellation_(cancellation)
    {
    }

  protected:
    Standard_Boolean UserBreak() override
    {
        return cancellation_ != nullptr && cancellation_->is_cancelled();
    }

    void Show(const Message_ProgressScope&, Standard_Boolean) override {}

  private:
    const StepTopologyCancellation* cancellation_;
};

IMPLEMENT_STANDARD_RTTIEXT(RenderCancellationProgress, Message_ProgressIndicator)

class SaturatingSize
{
  public:
    void add(std::size_t value)
    {
        value_ = value > std::numeric_limits<std::size_t>::max() - value_
                     ? std::numeric_limits<std::size_t>::max()
                     : value_ + value;
    }

    void add_product(std::size_t count, std::size_t item_size)
    {
        if (item_size != 0 && count > std::numeric_limits<std::size_t>::max() / item_size)
        {
            value_ = std::numeric_limits<std::size_t>::max();
            return;
        }
        add(count * item_size);
    }

    std::size_t value() const
    {
        return value_;
    }

  private:
    std::size_t value_ = 0;
};

double determinant(const std::array<double, 12>& matrix)
{
    return matrix[0] * (matrix[5] * matrix[10] - matrix[6] * matrix[9]) -
           matrix[1] * (matrix[4] * matrix[10] - matrix[6] * matrix[8]) +
           matrix[2] * (matrix[4] * matrix[9] - matrix[5] * matrix[8]);
}

bool valid_signed_rigid_transform(const std::array<double, 12>& matrix)
{
    for (double value : matrix)
    {
        if (!std::isfinite(value))
        {
            return false;
        }
    }
    constexpr double tolerance = 1.0e-9;
    for (std::size_t first = 0; first < 3; ++first)
    {
        for (std::size_t second = 0; second < 3; ++second)
        {
            double dot = 0.0;
            for (std::size_t column = 0; column < 3; ++column)
            {
                dot += matrix[first * 4U + column] * matrix[second * 4U + column];
            }
            const double expected = first == second ? 1.0 : 0.0;
            if (std::abs(dot - expected) > tolerance)
            {
                return false;
            }
        }
    }
    return std::abs(std::abs(determinant(matrix)) - 1.0) <= tolerance;
}

std::array<double, 12> multiply_transform(const std::array<double, 12>& first,
                                          const std::array<double, 12>& second)
{
    std::array<double, 12> result{};
    for (std::size_t row = 0; row < 3; ++row)
    {
        for (std::size_t column = 0; column < 3; ++column)
        {
            for (std::size_t inner = 0; inner < 3; ++inner)
            {
                result[row * 4U + column] += first[row * 4U + inner] * second[inner * 4U + column];
            }
        }
        result[row * 4U + 3U] = first[row * 4U + 3U];
        for (std::size_t inner = 0; inner < 3; ++inner)
        {
            result[row * 4U + 3U] += first[row * 4U + inner] * second[inner * 4U + 3U];
        }
    }
    return result;
}

std::size_t estimated_render_bytes(const StepTopologyRenderArtifact& artifact)
{
    SaturatingSize estimate;
    estimate.add(sizeof(artifact));
    estimate.add(artifact.research_format.capacity() + 1U);
    estimate.add(artifact.artifact_handle.capacity() + 1U);
    estimate.add(artifact.content_sha256.capacity() + 1U);
    estimate.add(artifact.normalized_length_unit.capacity() + 1U);
    estimate.add(artifact.session.session_handle.capacity() + 1U);
    estimate.add(artifact.session.source_sha256.capacity() + 1U);
    estimate.add(artifact.session.occt_version.capacity() + 1U);
    estimate.add_product(artifact.meshes.capacity(), sizeof(StepTopologyRenderMesh));
    for (const StepTopologyRenderMesh& mesh : artifact.meshes)
    {
        estimate.add(mesh.definition_handle.capacity() + 1U);
        estimate.add_product(mesh.vertices.capacity(), sizeof(StepTopologyRenderVertex));
        estimate.add_product(mesh.indices.capacity(), sizeof(std::uint32_t));
        estimate.add_product(mesh.primitives.capacity(), sizeof(StepTopologyRenderPrimitive));
        for (const StepTopologyRenderPrimitive& primitive : mesh.primitives)
        {
            estimate.add(primitive.body_handle.capacity() + 1U);
            estimate.add(primitive.face_handle.capacity() + 1U);
        }
    }
    estimate.add_product(artifact.instances.capacity(), sizeof(StepTopologyRenderInstance));
    for (const StepTopologyRenderInstance& instance : artifact.instances)
    {
        estimate.add(instance.occurrence_handle.capacity() + 1U);
        estimate.add(instance.definition_handle.capacity() + 1U);
    }
    estimate.add_product(artifact.bindings.capacity(), sizeof(StepTopologyTriangleBinding));
    for (const StepTopologyTriangleBinding& binding : artifact.bindings)
    {
        estimate.add(binding.occurrence_handle.capacity() + 1U);
        estimate.add(binding.body_handle.capacity() + 1U);
        estimate.add(binding.face_handle.capacity() + 1U);
    }
    return estimate.value();
}

void hash_u64(Sha256Builder* hash, std::uint64_t value)
{
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
        bytes[index] = static_cast<std::uint8_t>(value >> (index * 8U));
    }
    hash->update(bytes.data(), bytes.size());
}

void hash_double(Sha256Builder* hash, double value)
{
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    hash_u64(hash, bits);
}

void hash_string(Sha256Builder* hash, const std::string& value)
{
    hash_u64(hash, static_cast<std::uint64_t>(value.size()));
    hash->update(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
}

template <std::size_t Size>
void hash_doubles(Sha256Builder* hash, const std::array<double, Size>& values)
{
    for (double value : values)
    {
        hash_double(hash, value);
    }
}

std::string render_content_digest(const StepTopologyRenderArtifact& artifact)
{
    Sha256Builder hash;
    hash_string(&hash, "geometer.step_topology_render.content.a0");
    hash_string(&hash, artifact.research_format);
    hash_string(&hash, artifact.session.session_handle);
    hash_u64(&hash, artifact.session.generation);
    hash_string(&hash, artifact.session.source_sha256);
    hash_string(&hash, artifact.session.occt_version);
    hash_u64(&hash, artifact.session.source_bytes);
    hash_u64(&hash, artifact.session.edit_journal_revision);
    hash_u64(&hash, artifact.session.accounted_string_bytes);
    hash_u64(&hash, artifact.session.estimated_resident_bytes);
    hash_string(&hash, artifact.normalized_length_unit);
    hash_doubles(&hash, artifact.source_to_render);
    hash_u64(&hash, artifact.meshes.size());
    for (const StepTopologyRenderMesh& mesh : artifact.meshes)
    {
        hash_string(&hash, mesh.definition_handle);
        hash_u64(&hash, mesh.vertices.size());
        for (const StepTopologyRenderVertex& vertex : mesh.vertices)
        {
            hash_doubles(&hash, vertex.position);
            hash_doubles(&hash, vertex.normal);
        }
        hash_u64(&hash, mesh.indices.size());
        for (std::uint32_t index : mesh.indices)
        {
            hash_u64(&hash, index);
        }
        hash_u64(&hash, mesh.primitives.size());
        for (const StepTopologyRenderPrimitive& primitive : mesh.primitives)
        {
            hash_string(&hash, primitive.body_handle);
            hash_string(&hash, primitive.face_handle);
            hash_u64(&hash, primitive.first_index);
            hash_u64(&hash, primitive.index_count);
            hash_u64(&hash, primitive.first_triangle);
            hash_u64(&hash, primitive.triangle_count);
        }
    }
    hash_u64(&hash, artifact.instances.size());
    for (const StepTopologyRenderInstance& instance : artifact.instances)
    {
        hash_string(&hash, instance.occurrence_handle);
        hash_string(&hash, instance.definition_handle);
        hash_u64(&hash, instance.mesh_index);
        hash_doubles(&hash, instance.transform);
        hash_u64(&hash, instance.front_face_reversed ? 1U : 0U);
    }
    hash_u64(&hash, artifact.bindings.size());
    for (const StepTopologyTriangleBinding& binding : artifact.bindings)
    {
        hash_u64(&hash, binding.instance_index);
        hash_u64(&hash, binding.primitive_index);
        hash_u64(&hash, binding.first_triangle);
        hash_u64(&hash, binding.triangle_count);
        hash_string(&hash, binding.occurrence_handle);
        hash_string(&hash, binding.body_handle);
        hash_string(&hash, binding.face_handle);
    }
    hash_u64(&hash, artifact.geometry_triangle_count);
    hash_u64(&hash, artifact.instanced_triangle_count);
    return hash.hex_digest();
}

std::string render_artifact_handle(const SessionData& data, const std::string& content_digest)
{
    Sha256Builder hash;
    hash.update(data.secret.data(), data.secret.size());
    hash_string(&hash, "geometer.step_topology_render.seal.a0");
    hash_u64(&hash, data.info.generation);
    hash_string(&hash, content_digest);
    return "gtr_" + hash.hex_digest();
}

class RenderBuilder
{
  public:
    RenderBuilder(SessionData* data, const StepTopologyTessellationOptions& options,
                  const StepTopologyCancellation* cancellation, std::size_t byte_limit,
                  StepTopologyRenderArtifact* artifact, Status* status)
        : data_(data), options_(options), cancellation_(cancellation), artifact_(artifact),
          status_(status),
          byte_limit_(std::min(byte_limit, data->limits.max_render_estimated_bytes))
    {
    }

    int build()
    {
        if (!valid_options())
        {
            return status_code_;
        }
        artifact_->session = data_->info;
        artifact_->source_to_render = options_.source_to_render;
        if (!charge(sizeof(*artifact_) * 2U + artifact_->research_format.size() +
                        artifact_->normalized_length_unit.size() +
                        data_->info.session_handle.size() + data_->info.source_sha256.size() +
                        data_->info.occt_version.size(),
                    "render artifact transient bytes"))
            return status_code_;
        std::unordered_map<std::string, std::size_t> definition_meshes;
        for (const StepTopologyDefinition& definition : data_->snapshot.definitions)
        {
            if (cancel_requested())
            {
                return status_code_;
            }
            if (definition.body_handles.empty())
            {
                continue;
            }
            if (!bounded(artifact_->meshes.size() + 1U, data_->limits.max_definitions,
                         "render mesh count"))
            {
                return status_code_;
            }
            const auto shape = data_->handles.find(definition.handle);
            if (shape == data_->handles.end() ||
                shape->second.kind != StepTopologyTargetKind::definition)
            {
                fail(kInternalFailure, "Render definition handle has no live shape.");
                return status_code_;
            }
            StepTopologyRenderMesh mesh;
            mesh.definition_handle = definition.handle;
            if (!charge(sizeof(StepTopologyRenderMesh) * 2U + definition.handle.size() * 2U,
                        "render mesh transient bytes"))
                return status_code_;
            if (!build_mesh(definition, shape->second.shape, &mesh))
            {
                return status_code_;
            }
            definition_meshes.emplace(definition.handle, artifact_->meshes.size());
            artifact_->geometry_triangle_count += mesh.indices.size() / 3U;
            artifact_->meshes.push_back(std::move(mesh));
        }
        for (const StepTopologyRootOccurrence& occurrence : data_->snapshot.root_occurrences)
        {
            if (!append_instance(occurrence.handle, occurrence.definition_handle,
                                 occurrence.transform, definition_meshes))
            {
                return status_code_;
            }
        }
        for (const StepTopologyOccurrence& occurrence : data_->snapshot.occurrences)
        {
            if (!append_instance(occurrence.handle, occurrence.definition_handle,
                                 occurrence.transform, definition_meshes))
            {
                return status_code_;
            }
        }
        artifact_->content_sha256 = render_content_digest(*artifact_);
        artifact_->artifact_handle = render_artifact_handle(*data_, artifact_->content_sha256);
        artifact_->estimated_resident_bytes = estimated_render_bytes(*artifact_);
        if (artifact_->estimated_resident_bytes > byte_limit_)
        {
            fail(kResourceLimit, "STEP topology render artifact exceeds its byte limit.");
            return status_code_;
        }
        set_status(status_, 0, "");
        return 0;
    }

  private:
    bool valid_options()
    {
        if (!std::isfinite(options_.linear_deflection) || options_.linear_deflection <= 0.0 ||
            !std::isfinite(options_.angular_deflection) || options_.angular_deflection <= 0.0 ||
            options_.angular_deflection > 3.14159265358979323846 ||
            !valid_signed_rigid_transform(options_.source_to_render))
        {
            return fail(kInvalidArgument,
                        "STEP topology tessellation options require positive deflection, an "
                        "angle in (0, pi], and a signed-rigid coordinate transform.");
        }
        return !cancel_requested();
    }

    bool build_mesh(const StepTopologyDefinition& definition, const TopoDS_Shape& shape,
                    StepTopologyRenderMesh* mesh)
    {
        BRepTools::Clean(shape);
        IMeshTools_Parameters parameters;
        parameters.Deflection = options_.linear_deflection;
        parameters.Angle = options_.angular_deflection;
        parameters.Relative = options_.relative;
        parameters.InParallel = options_.parallel;
        Handle(RenderCancellationProgress) progress = new RenderCancellationProgress(cancellation_);
        BRepMesh_IncrementalMesh mesher(shape, parameters, progress->Start());
        if (cancel_requested())
        {
            return false;
        }
        if (!mesher.IsDone())
        {
            return fail(kTransferFailed, "OCCT failed tessellating a STEP topology definition.");
        }
        for (const StepTopologyFace& face : data_->snapshot.faces)
        {
            if (face.definition_handle != definition.handle)
            {
                continue;
            }
            if (face.body_handles.size() != 1U)
            {
                return fail(kTransferFailed,
                            "A selectable render face must belong to exactly one body.");
            }
            const auto known = data_->handles.find(face.handle);
            if (known == data_->handles.end() || known->second.kind != StepTopologyTargetKind::face)
            {
                return fail(kInternalFailure, "Render face handle has no live shape.");
            }
            if (!append_face(face, TopoDS::Face(known->second.shape), mesh))
            {
                return false;
            }
        }
        return true;
    }

    bool append_face(const StepTopologyFace& face, const TopoDS_Face& shape,
                     StepTopologyRenderMesh* mesh)
    {
        if (cancel_requested())
        {
            return false;
        }
        TopLoc_Location location;
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(shape, location);
        if (triangulation.IsNull() || triangulation->NbTriangles() <= 0)
        {
            return fail(kTransferFailed, "A STEP topology face has no tessellation triangles.");
        }
        triangulation->ComputeNormals();
        const std::size_t node_count = static_cast<std::size_t>(triangulation->NbNodes());
        const std::size_t triangle_count = static_cast<std::size_t>(triangulation->NbTriangles());
        SaturatingSize face_bytes;
        face_bytes.add_product(node_count, sizeof(StepTopologyRenderVertex) * 2U);
        face_bytes.add_product(triangle_count * 3U, sizeof(std::uint32_t) * 2U);
        face_bytes.add(sizeof(StepTopologyRenderPrimitive) * 2U);
        face_bytes.add((face.body_handles.front().size() + face.handle.size() + 2U) * 2U);
        if (!charge(face_bytes.value(), "render face transient bytes"))
            return false;
        if (!bounded(total_vertices_ + node_count, data_->limits.max_render_vertices,
                     "render vertex count") ||
            !bounded(total_indices_ + triangle_count * 3U, data_->limits.max_render_indices,
                     "render index count") ||
            !bounded(total_primitives_ + 1U, data_->limits.max_render_primitives,
                     "render primitive count"))
        {
            return false;
        }
        if (mesh->vertices.size() + node_count >
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        {
            return fail(kResourceLimit, "A render mesh exceeds the 32-bit index range.");
        }
        const std::size_t first_vertex = mesh->vertices.size();
        const gp_Trsf& transform = location.Transformation();
        const bool reversed = shape.Orientation() == TopAbs_REVERSED;
        mesh->vertices.reserve(first_vertex + node_count);
        for (int node = 1; node <= triangulation->NbNodes(); ++node)
        {
            gp_Pnt point = triangulation->Node(node);
            point.Transform(transform);
            gp_Vec normal(triangulation->Normal(node));
            normal.Transform(transform);
            if (reversed)
            {
                normal.Reverse();
            }
            normal.Normalize();
            mesh->vertices.push_back(
                {{point.X(), point.Y(), point.Z()}, {normal.X(), normal.Y(), normal.Z()}});
        }
        StepTopologyRenderPrimitive primitive;
        primitive.body_handle = face.body_handles.front();
        primitive.face_handle = face.handle;
        primitive.first_index = mesh->indices.size();
        primitive.first_triangle = mesh->indices.size() / 3U;
        primitive.index_count = triangle_count * 3U;
        primitive.triangle_count = triangle_count;
        mesh->indices.reserve(mesh->indices.size() + primitive.index_count);
        for (int triangle = 1; triangle <= triangulation->NbTriangles(); ++triangle)
        {
            if (cancel_requested())
            {
                return false;
            }
            int first = 0;
            int second = 0;
            int third = 0;
            triangulation->Triangle(triangle).Get(first, second, third);
            if (reversed)
            {
                std::swap(second, third);
            }
            for (int node : {first, second, third})
            {
                mesh->indices.push_back(
                    static_cast<std::uint32_t>(first_vertex + static_cast<std::size_t>(node - 1)));
            }
        }
        mesh->primitives.push_back(std::move(primitive));
        total_vertices_ += node_count;
        total_indices_ += triangle_count * 3U;
        ++total_primitives_;
        return true;
    }

    bool append_instance(const std::string& occurrence_handle, const std::string& definition_handle,
                         const std::array<double, 12>& occurrence_transform,
                         const std::unordered_map<std::string, std::size_t>& definition_meshes)
    {
        const auto mesh = definition_meshes.find(definition_handle);
        if (mesh == definition_meshes.end())
        {
            return true;
        }
        if (cancel_requested() ||
            !bounded(artifact_->instances.size() + 1U, data_->limits.max_render_instances,
                     "render instance count"))
        {
            return false;
        }
        const StepTopologyRenderMesh& geometry = artifact_->meshes[mesh->second];
        if (!bounded(artifact_->bindings.size() + geometry.primitives.size(),
                     data_->limits.max_render_bindings, "render binding count"))
        {
            return false;
        }
        SaturatingSize instance_bytes;
        instance_bytes.add(sizeof(StepTopologyRenderInstance) * 2U);
        instance_bytes.add((occurrence_handle.size() + definition_handle.size() + 2U) * 2U);
        for (const auto& primitive : geometry.primitives)
        {
            instance_bytes.add(sizeof(StepTopologyTriangleBinding) * 2U);
            instance_bytes.add((occurrence_handle.size() + primitive.body_handle.size() +
                                primitive.face_handle.size() + 3U) *
                               2U);
        }
        if (!charge(instance_bytes.value(), "render instance transient bytes"))
            return false;
        StepTopologyRenderInstance instance;
        instance.occurrence_handle = occurrence_handle;
        instance.definition_handle = definition_handle;
        instance.mesh_index = mesh->second;
        instance.transform = multiply_transform(options_.source_to_render, occurrence_transform);
        instance.front_face_reversed = determinant(instance.transform) < 0.0;
        const std::size_t instance_index = artifact_->instances.size();
        artifact_->instances.push_back(instance);
        for (std::size_t primitive_index = 0; primitive_index < geometry.primitives.size();
             ++primitive_index)
        {
            const StepTopologyRenderPrimitive& primitive = geometry.primitives[primitive_index];
            artifact_->bindings.push_back({instance_index, primitive_index,
                                           primitive.first_triangle, primitive.triangle_count,
                                           occurrence_handle, primitive.body_handle,
                                           primitive.face_handle});
            if (primitive.triangle_count >
                    std::numeric_limits<std::size_t>::max() - artifact_->instanced_triangle_count ||
                artifact_->instanced_triangle_count + primitive.triangle_count >
                    data_->limits.max_render_instanced_triangles)
            {
                return fail(kResourceLimit, "Instanced render triangle count exceeds its limit.");
            }
            artifact_->instanced_triangle_count += primitive.triangle_count;
        }
        return true;
    }

    bool cancel_requested()
    {
        if (cancellation_ == nullptr || !cancellation_->is_cancelled())
        {
            return false;
        }
        fail(kCancelled, "STEP topology tessellation was cancelled.");
        return true;
    }

    bool bounded(std::size_t value, std::size_t limit, const char* what)
    {
        return value <= limit ? true
                              : fail(kResourceLimit,
                                     std::string("STEP topology ") + what + " exceeds its limit.");
    }

    bool charge(std::size_t value, const char* what)
    {
        if (value > byte_limit_ || charged_bytes_ > byte_limit_ - value)
            return fail(kResourceLimit,
                        std::string("STEP topology ") + what + " exceeds its limit.");
        charged_bytes_ += value;
        return true;
    }

    bool fail(int code, const std::string& message)
    {
        status_code_ = code;
        set_status(status_, code, message);
        return false;
    }

    SessionData* data_;
    const StepTopologyTessellationOptions& options_;
    const StepTopologyCancellation* cancellation_;
    StepTopologyRenderArtifact* artifact_;
    Status* status_;
    int status_code_ = 0;
    std::size_t total_primitives_ = 0;
    std::size_t total_vertices_ = 0;
    std::size_t total_indices_ = 0;
    std::size_t byte_limit_ = 0;
    std::size_t charged_bytes_ = 0;
};

} // namespace

int build_render_artifact(SessionData* data, const StepTopologyTessellationOptions& options,
                          const StepTopologyCancellation* cancellation, std::size_t byte_limit,
                          StepTopologyRenderArtifact* artifact, Status* status)
{
    try
    {
        RenderBuilder builder(data, options, cancellation, byte_limit, artifact, status);
        const int code = builder.build();
        if (code != 0)
        {
            *artifact = {};
        }
        return code;
    }
    catch (const Standard_Failure& failure)
    {
        *artifact = {};
        set_status(status, kInternalFailure, failure.GetMessageString());
        return kInternalFailure;
    }
    catch (const std::exception& error)
    {
        *artifact = {};
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

bool verify_render_artifact_seal(const SessionData* data,
                                 const StepTopologyRenderArtifact& artifact, Status* status)
{
    const std::string content_digest = render_content_digest(artifact);
    const std::string expected_handle = render_artifact_handle(*data, content_digest);
    if (artifact.content_sha256 != content_digest || artifact.artifact_handle != expected_handle)
    {
        set_status(status, kUnknownTarget,
                   "STEP topology render artifact content or session seal is invalid.");
        return false;
    }
    return true;
}

} // namespace geometer::step_topology_internal
