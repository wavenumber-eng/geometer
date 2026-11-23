#include "model_illustration_source_cache.h"

#include <limits>

namespace geometer::model_illustration_detail
{
namespace
{
class StorageCount
{
  public:
    void add(std::size_t bytes)
    {
        const auto maximum = std::numeric_limits<std::size_t>::max();
        total_ = bytes > maximum - total_ ? maximum : total_ + bytes;
    }

    template <typename T> void allocation(std::size_t capacity)
    {
        const auto maximum = std::numeric_limits<std::size_t>::max();
        if (capacity > maximum / sizeof(T))
            add(maximum);
        else
            add(capacity * sizeof(T));
        // Conservative allocator allowance (also counts empty/SSO storage).
        add(64);
    }

    void string(const std::string& value)
    {
        allocation<char>(value.capacity());
        add(1);
    }

    std::size_t total() const
    {
        return total_;
    }

  private:
    std::size_t total_ = 0;
};
} // namespace

std::size_t illustration_source_cache_owned_bytes(std::size_t raw_capacity,
                                                  const std::string& options,
                                                  const contracts::MeshCollectionA0& collection,
                                                  const std::vector<std::string>& warnings)
{
    StorageCount count;
    // Entry, list node, shared-owner allocation and inline container fields.
    count.add(512 + sizeof(collection) + sizeof(options) + sizeof(warnings));
    count.allocation<unsigned char>(raw_capacity);
    count.string(options);
    count.string(collection.schema);
    count.string(collection.length_unit);
    count.allocation<contracts::MeshIllustrationMesh>(collection.meshes.capacity());
    for (const auto& mesh : collection.meshes)
    {
        count.string(mesh.id);
        count.allocation<double>(mesh.positions.capacity());
        if (mesh.normals)
            count.allocation<double>(mesh.normals->capacity());
        if (mesh.indices)
            count.allocation<std::uint32_t>(mesh.indices->capacity());
        if (mesh.matrix)
            count.allocation<double>(mesh.matrix->capacity());
        if (mesh.triangle_material_indices)
            count.allocation<std::uint32_t>(mesh.triangle_material_indices->capacity());
        count.allocation<contracts::MeshIllustrationMaterial>(mesh.materials.capacity());
        for (const auto& material : mesh.materials)
        {
            count.allocation<double>(material.color.capacity());
            if (material.name)
                count.string(*material.name);
        }
    }
    count.allocation<std::string>(warnings.capacity());
    for (const auto& warning : warnings)
        count.string(warning);
    return count.total();
}
} // namespace geometer::model_illustration_detail
