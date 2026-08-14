#include "geometer/exact_artifact.h"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace geometer::exact
{
namespace
{

class StorageReservation
{
  public:
    StorageReservation(Budget& budget, std::uint64_t bytes)
        : budget_(budget), bytes_(bytes), acquired_(budget.acquire_storage(bytes))
    {
    }
    ~StorageReservation()
    {
        if (acquired_ && !committed_)
            budget_.release_storage(bytes_);
    }
    [[nodiscard]] bool acquired() const
    {
        return acquired_;
    }
    void commit()
    {
        committed_ = true;
    }

  private:
    Budget& budget_;
    std::uint64_t bytes_;
    bool acquired_;
    bool committed_ = false;
};

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("construction artifact size overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("construction artifact size overflow");
    return left * right;
}

std::uint64_t align(std::uint64_t value, std::uint64_t boundary)
{
    return checked_multiply(checked_add(value, boundary - 1) / boundary, boundary);
}

void append_u16(std::vector<std::uint8_t>& output, std::uint16_t value)
{
    output.push_back(static_cast<std::uint8_t>(value & 0xff));
    output.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void append_u32(std::vector<std::uint8_t>& output, std::uint32_t value)
{
    for (std::uint32_t shift = 0; shift < 32; shift += 8)
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
}

void append_u64(std::vector<std::uint8_t>& output, std::uint64_t value)
{
    for (std::uint32_t shift = 0; shift < 64; shift += 8)
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
}

bool read_u16(const std::vector<std::uint8_t>& input, std::size_t& offset, std::uint16_t& value)
{
    if (offset > input.size() || input.size() - offset < 2)
        return false;
    value = static_cast<std::uint16_t>(input[offset]) |
            static_cast<std::uint16_t>(input[offset + 1]) << 8;
    offset += 2;
    return true;
}

bool read_u32(const std::vector<std::uint8_t>& input, std::size_t& offset, std::uint32_t& value)
{
    if (offset > input.size() || input.size() - offset < 4)
        return false;
    value = 0;
    for (std::uint32_t shift = 0; shift < 32; shift += 8)
        value |= static_cast<std::uint32_t>(input[offset++]) << shift;
    return true;
}

bool read_u64(const std::vector<std::uint8_t>& input, std::size_t& offset, std::uint64_t& value)
{
    if (offset > input.size() || input.size() - offset < 8)
        return false;
    value = 0;
    for (std::uint32_t shift = 0; shift < 64; shift += 8)
        value |= static_cast<std::uint64_t>(input[offset++]) << shift;
    return true;
}

bool valid_kind(std::uint8_t kind, std::uint32_t child_count)
{
    switch (static_cast<ConstructionKind>(kind))
    {
    case ConstructionKind::rational:
        return child_count == 0;
    case ConstructionKind::sum:
    case ConstructionKind::product:
        return child_count >= 2;
    case ConstructionKind::reciprocal:
    case ConstructionKind::nonnegative_square_root:
        return child_count == 1;
    }
    return false;
}

ConstructionResult rebuild_node(ConstructionArena& arena, ConstructionKind kind,
                                const CanonicalReal& decoded_value,
                                const std::vector<ConstructionNodeId>& children)
{
    switch (kind)
    {
    case ConstructionKind::rational:
        if (decoded_value.kind() != CanonicalRealKind::rational)
            return {Error::invalid_argument, std::nullopt};
        return arena.make_rational(decoded_value.numerator(), decoded_value.denominator());
    case ConstructionKind::sum:
        return arena.make_sum(children);
    case ConstructionKind::product:
        return arena.make_product(children);
    case ConstructionKind::reciprocal:
        return arena.make_reciprocal(children.front());
    case ConstructionKind::nonnegative_square_root:
        return arena.make_nonnegative_square_root(children.front());
    }
    return {Error::invalid_argument, std::nullopt};
}

Error canonical_node_order(Budget& budget, const ConstructionArena& arena, bool& canonical)
{
    canonical = false;
    std::uint64_t key_bytes = 0;
    for (std::size_t index = 0; index < arena.size(); ++index)
        key_bytes = checked_add(
            key_bytes, arena.at(static_cast<ConstructionNodeId>(index)).construction_key().size());
    const std::uint64_t comparison_work = checked_multiply(
        2, checked_multiply(checked_add(key_bytes, 1), checked_add(arena.size(), 1)));
    if (!budget.consume_work(comparison_work))
        return Error::resource_limit_exceeded;
    std::vector<ConstructionNodeId> order(arena.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&](ConstructionNodeId left, ConstructionNodeId right)
              {
                  const ConstructionNode& lhs = arena.at(left);
                  const ConstructionNode& rhs = arena.at(right);
                  if (lhs.depth() != rhs.depth())
                      return lhs.depth() < rhs.depth();
                  return lhs.construction_key() < rhs.construction_key();
              });
    for (std::size_t index = 0; index < order.size(); ++index)
        if (order[index] != index)
            return Error::none;
    canonical = true;
    return Error::none;
}

} // namespace

DecodedConstructionArtifact::DecodedConstructionArtifact(Budget& budget,
                                                         std::uint64_t charged_bytes,
                                                         std::unique_ptr<ConstructionArena> arena,
                                                         std::vector<ConstructionNodeId> roots)
    : budget_(&budget), charged_bytes_(charged_bytes), arena_(std::move(arena)),
      roots_(std::move(roots))
{
}

DecodedConstructionArtifact::~DecodedConstructionArtifact()
{
    arena_.reset();
    roots_.clear();
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
}

DecodedConstructionArtifact::DecodedConstructionArtifact(
    DecodedConstructionArtifact&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), arena_(std::move(other.arena_)),
      roots_(std::move(other.roots_))
{
}

DecodedConstructionArtifact&
DecodedConstructionArtifact::operator=(DecodedConstructionArtifact&& other) noexcept
{
    if (this != &other)
    {
        arena_.reset();
        roots_.clear();
        if (budget_ != nullptr)
            budget_->release_storage(charged_bytes_);
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        arena_ = std::move(other.arena_);
        roots_ = std::move(other.roots_);
    }
    return *this;
}

const ConstructionArena& DecodedConstructionArtifact::arena() const
{
    return *arena_;
}

const std::vector<ConstructionNodeId>& DecodedConstructionArtifact::roots() const
{
    return roots_;
}

EncodeResult encode_construction_artifact(Budget& budget, const ConstructionArena& arena,
                                          const std::vector<ConstructionNodeId>& ordered_roots)
{
    for (const ConstructionNodeId root : ordered_roots)
        if (static_cast<std::size_t>(root) >= arena.size())
            return {Error::invalid_argument, std::nullopt};
    if (ordered_roots.size() > std::numeric_limits<std::uint32_t>::max())
        return {Error::resource_limit_exceeded, std::nullopt};
    try
    {
        std::uint64_t key_bytes = 0;
        std::uint64_t edge_count = 0;
        for (std::size_t index = 0; index < arena.size(); ++index)
        {
            const ConstructionNode& node = arena.at(static_cast<ConstructionNodeId>(index));
            key_bytes = checked_add(key_bytes, node.construction_key().size());
            edge_count = checked_add(edge_count, node.children().size());
        }
        const std::uint64_t planning_storage =
            checked_add(4096, checked_add(checked_multiply(arena.size(), 64),
                                          checked_add(checked_multiply(edge_count, 8),
                                                      checked_multiply(ordered_roots.size(), 16))));
        const std::uint64_t planning_work = checked_add(
            checked_add(checked_multiply(arena.size(), 64),
                        checked_multiply(checked_add(edge_count, ordered_roots.size()), 16)),
            checked_multiply(
                2, checked_multiply(checked_add(key_bytes, 1), checked_add(arena.size(), 1))));
        StorageReservation planning(budget, planning_storage);
        if (!planning.acquired() || !budget.consume_work(planning_work))
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<std::uint8_t> reachable(arena.size(), 0);
        std::vector<ConstructionNodeId> stack(ordered_roots.begin(), ordered_roots.end());
        while (!stack.empty())
        {
            const ConstructionNodeId node = stack.back();
            stack.pop_back();
            if (reachable[node] != 0)
                continue;
            reachable[node] = 1;
            const auto& children = arena.at(node).children();
            stack.insert(stack.end(), children.begin(), children.end());
        }
        std::vector<ConstructionNodeId> order;
        for (std::size_t index = 0; index < arena.size(); ++index)
            if (reachable[index] != 0)
                order.push_back(static_cast<ConstructionNodeId>(index));
        if (order.size() > std::numeric_limits<std::uint32_t>::max())
            return {Error::resource_limit_exceeded, std::nullopt};
        std::sort(order.begin(), order.end(),
                  [&](ConstructionNodeId left, ConstructionNodeId right)
                  {
                      const ConstructionNode& lhs = arena.at(left);
                      const ConstructionNode& rhs = arena.at(right);
                      if (lhs.depth() != rhs.depth())
                          return lhs.depth() < rhs.depth();
                      return lhs.construction_key() < rhs.construction_key();
                  });
        std::vector<ConstructionNodeId> remap(arena.size(), 0);
        for (std::size_t index = 0; index < order.size(); ++index)
            remap[order[index]] = static_cast<ConstructionNodeId>(index);
        std::uint64_t total = 32;
        for (const ConstructionNodeId id : order)
        {
            const ConstructionNode& node = arena.at(id);
            const std::uint64_t payload =
                checked_add(24, checked_add(checked_multiply(node.children().size(), 4),
                                            node.value_key().size()));
            const std::uint64_t record = align(payload, 8);
            if (record > std::numeric_limits<std::uint32_t>::max())
                return {Error::resource_limit_exceeded, std::nullopt};
            total = checked_add(total, record);
        }
        total = checked_add(total, checked_multiply(ordered_roots.size(), 4));
        if (total > std::numeric_limits<std::size_t>::max())
            return {Error::resource_limit_exceeded, std::nullopt};
        const std::uint64_t output_storage = checked_add(256, checked_multiply(total, 2));
        StorageReservation output_reservation(budget, output_storage);
        if (!output_reservation.acquired() || !budget.consume_work(checked_multiply(total, 8)))
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<std::uint8_t> output;
        output.reserve(static_cast<std::size_t>(total));
        constexpr std::array<std::uint8_t, 8> magic = {'G', 'E', 'X', 'P', 'A', '0', '0', '1'};
        output.insert(output.end(), magic.begin(), magic.end());
        append_u16(output, 1);
        append_u16(output, 0);
        append_u32(output, static_cast<std::uint32_t>(order.size()));
        append_u32(output, static_cast<std::uint32_t>(ordered_roots.size()));
        append_u32(output, 0);
        append_u64(output, total);
        for (const ConstructionNodeId id : order)
        {
            const ConstructionNode& node = arena.at(id);
            const std::uint64_t payload =
                checked_add(24, checked_add(checked_multiply(node.children().size(), 4),
                                            node.value_key().size()));
            const auto record = static_cast<std::uint32_t>(align(payload, 8));
            append_u32(output, record);
            output.push_back(static_cast<std::uint8_t>(node.kind()));
            output.push_back(0);
            append_u16(output, 0);
            append_u32(output, static_cast<std::uint32_t>(node.children().size()));
            append_u32(output, static_cast<std::uint32_t>(node.value_key().size()));
            append_u32(output, 0);
            append_u32(output, 0);
            for (const ConstructionNodeId child : node.children())
                append_u32(output, remap[child]);
            output.insert(output.end(), node.value_key().begin(), node.value_key().end());
            output.resize(output.size() + (record - payload), 0);
        }
        for (const ConstructionNodeId root : ordered_roots)
            append_u32(output, remap[root]);
        if (output.size() != total)
            return {Error::resource_limit_exceeded, std::nullopt};
        output_reservation.commit();
        return {Error::none, EncodedBytes(budget, output_storage, std::move(output))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

DecodeConstructionArtifactResult
decode_construction_artifact(Budget& budget, const std::vector<std::uint8_t>& input)
{
    if (input.size() < 32)
        return {Error::invalid_argument, std::nullopt};
    try
    {
        const std::uint64_t parse_storage = checked_add(4096, checked_multiply(input.size(), 4));
        StorageReservation parsing(budget, parse_storage);
        if (!parsing.acquired() || !budget.consume_work(checked_multiply(input.size(), 16)))
            return {Error::resource_limit_exceeded, std::nullopt};
        constexpr std::array<std::uint8_t, 8> magic = {'G', 'E', 'X', 'P', 'A', '0', '0', '1'};
        if (!std::equal(magic.begin(), magic.end(), input.begin()))
            return {Error::invalid_argument, std::nullopt};
        std::size_t offset = 8;
        std::uint16_t generation = 0;
        std::uint16_t flags = 0;
        std::uint32_t node_count = 0;
        std::uint32_t root_count = 0;
        std::uint32_t reserved = 0;
        std::uint64_t total_bytes = 0;
        if (!read_u16(input, offset, generation) || !read_u16(input, offset, flags) ||
            !read_u32(input, offset, node_count) || !read_u32(input, offset, root_count) ||
            !read_u32(input, offset, reserved) || !read_u64(input, offset, total_bytes) ||
            generation != 1 || flags != 0 || reserved != 0 || total_bytes != input.size())
            return {Error::invalid_argument, std::nullopt};
        const std::uint64_t root_bytes = checked_multiply(root_count, 4);
        if (root_bytes > input.size() - offset ||
            node_count > (input.size() - offset - root_bytes) / 24)
            return {Error::invalid_argument, std::nullopt};
        const std::size_t root_table = input.size() - static_cast<std::size_t>(root_bytes);
        auto arena = std::make_unique<ConstructionArena>(budget);
        for (std::uint32_t index = 0; index < node_count; ++index)
        {
            const std::size_t record_start = offset;
            std::uint32_t record_bytes = 0;
            if (!read_u32(input, offset, record_bytes) || record_bytes < 24 ||
                record_bytes % 8 != 0 || record_bytes > root_table - record_start)
                return {Error::invalid_argument, std::nullopt};
            const std::size_t record_end = record_start + record_bytes;
            if (offset > record_end || record_end - offset < 20)
                return {Error::invalid_argument, std::nullopt};
            const std::uint8_t kind_value = input[offset++];
            const std::uint8_t record_flags = input[offset++];
            std::uint16_t record_reserved = 0;
            std::uint32_t child_count = 0;
            std::uint32_t scalar_bytes = 0;
            std::uint32_t reserved_one = 0;
            std::uint32_t reserved_two = 0;
            if (!read_u16(input, offset, record_reserved) ||
                !read_u32(input, offset, child_count) || !read_u32(input, offset, scalar_bytes) ||
                !read_u32(input, offset, reserved_one) || !read_u32(input, offset, reserved_two) ||
                record_flags != 0 || record_reserved != 0 || reserved_one != 0 ||
                reserved_two != 0 || !valid_kind(kind_value, child_count) || scalar_bytes < 8 ||
                scalar_bytes % 8 != 0)
                return {Error::invalid_argument, std::nullopt};
            const std::uint64_t payload =
                checked_add(24, checked_add(checked_multiply(child_count, 4), scalar_bytes));
            if (align(payload, 8) != record_bytes || payload > record_bytes)
                return {Error::invalid_argument, std::nullopt};
            std::vector<ConstructionNodeId> children;
            children.reserve(child_count);
            for (std::uint32_t child_index = 0; child_index < child_count; ++child_index)
            {
                std::uint32_t child = 0;
                if (!read_u32(input, offset, child) || child >= index)
                    return {Error::invalid_argument, std::nullopt};
                children.push_back(child);
            }
            if (scalar_bytes > record_end - offset)
                return {Error::invalid_argument, std::nullopt};
            std::vector<std::uint8_t> scalar(
                input.begin() + static_cast<std::ptrdiff_t>(offset),
                input.begin() + static_cast<std::ptrdiff_t>(offset + scalar_bytes));
            offset += scalar_bytes;
            while (offset < record_end)
                if (input[offset++] != 0)
                    return {Error::invalid_argument, std::nullopt};
            auto decoded = decode_canonical_real(budget, scalar);
            if (decoded.error != Error::none || !decoded.value)
                return {decoded.error, std::nullopt};
            ConstructionResult rebuilt = rebuild_node(
                *arena, static_cast<ConstructionKind>(kind_value), *decoded.value, children);
            if (rebuilt.error != Error::none || !rebuilt.node)
                return {rebuilt.error, std::nullopt};
            if (*rebuilt.node != index || arena->size() != static_cast<std::size_t>(index) + 1)
                return {Error::invalid_argument, std::nullopt};
            const ConstructionNode& node = arena->at(*rebuilt.node);
            if (node.kind() != static_cast<ConstructionKind>(kind_value) ||
                node.children() != children || node.value_key() != scalar)
                return {Error::invalid_argument, std::nullopt};
        }
        if (offset != root_table)
            return {Error::invalid_argument, std::nullopt};
        std::vector<ConstructionNodeId> roots;
        roots.reserve(root_count);
        for (std::uint32_t index = 0; index < root_count; ++index)
        {
            std::uint32_t root = 0;
            if (!read_u32(input, offset, root) || root >= node_count)
                return {Error::invalid_argument, std::nullopt};
            roots.push_back(root);
        }
        if (offset != input.size())
            return {Error::invalid_argument, std::nullopt};
        bool canonical_order = false;
        const Error order_error = canonical_node_order(budget, *arena, canonical_order);
        if (order_error != Error::none)
            return {order_error, std::nullopt};
        if (!canonical_order)
            return {Error::invalid_argument, std::nullopt};
        std::vector<std::uint8_t> reachable(node_count, 0);
        std::vector<ConstructionNodeId> stack(roots.begin(), roots.end());
        while (!stack.empty())
        {
            const ConstructionNodeId node = stack.back();
            stack.pop_back();
            if (reachable[node] != 0)
                continue;
            reachable[node] = 1;
            const auto& children = arena->at(node).children();
            stack.insert(stack.end(), children.begin(), children.end());
        }
        if (std::find(reachable.begin(), reachable.end(), 0) != reachable.end())
            return {Error::invalid_argument, std::nullopt};
        const std::uint64_t retained = checked_add(256, checked_multiply(roots.size(), 16));
        StorageReservation retained_reservation(budget, retained);
        if (!retained_reservation.acquired())
            return {Error::resource_limit_exceeded, std::nullopt};
        DecodedConstructionArtifact artifact(budget, retained, std::move(arena), std::move(roots));
        retained_reservation.commit();
        return {Error::none, std::move(artifact)};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

} // namespace geometer::exact
