#include "geometer/exact_construction.h"

#include <algorithm>
#include <limits>
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
        throw std::overflow_error("construction budget estimate overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("construction budget estimate overflow");
    return left * right;
}

void append_u32(std::vector<std::uint8_t>& output, std::uint32_t value)
{
    for (std::uint32_t shift = 0; shift < 32; shift += 8)
        output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xff));
}

EncodeResult make_construction_key(Budget& budget, ConstructionKind kind,
                                   const std::vector<ConstructionNodeId>& children,
                                   const std::vector<ConstructionNode>& nodes,
                                   const EncodedBytes& value_key)
{
    try
    {
        if (children.size() > std::numeric_limits<std::uint32_t>::max())
            return {Error::resource_limit_exceeded, std::nullopt};
        std::uint64_t size = 5;
        for (const ConstructionNodeId child : children)
            size = checked_add(size, checked_add(4, nodes[child].construction_key().size()));
        size = checked_add(size, checked_add(4, value_key.bytes().size()));
        if (size > std::numeric_limits<std::uint32_t>::max())
            return {Error::resource_limit_exceeded, std::nullopt};
        const std::uint64_t storage = checked_add(256, checked_multiply(size, 2));
        const std::uint64_t work = checked_multiply(size, 8);
        StorageReservation reservation(budget, storage);
        if (!reservation.acquired() || !budget.consume_work(work))
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<std::uint8_t> output;
        output.reserve(static_cast<std::size_t>(size));
        output.push_back(static_cast<std::uint8_t>(kind));
        append_u32(output, static_cast<std::uint32_t>(children.size()));
        for (const ConstructionNodeId child : children)
        {
            const auto& key = nodes[child].construction_key();
            append_u32(output, static_cast<std::uint32_t>(key.size()));
            output.insert(output.end(), key.begin(), key.end());
        }
        append_u32(output, static_cast<std::uint32_t>(value_key.bytes().size()));
        output.insert(output.end(), value_key.bytes().begin(), value_key.bytes().end());
        reservation.commit();
        return {Error::none, EncodedBytes(budget, storage, std::move(output))};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

bool key_less(const ConstructionNode& left, const ConstructionNode& right)
{
    if (left.value_key() != right.value_key())
        return left.value_key() < right.value_key();
    return left.construction_key() < right.construction_key();
}

bool is_identity(const CanonicalReal& value, ConstructionKind kind)
{
    return value.kind() == CanonicalRealKind::rational && value.denominator() == 1 &&
           value.numerator() == (kind == ConstructionKind::sum ? 0 : 1);
}

} // namespace

ConstructionNode::ConstructionNode(ConstructionKind kind, std::vector<ConstructionNodeId> children,
                                   CanonicalReal value, EncodedBytes value_key,
                                   EncodedBytes construction_key, std::uint32_t depth,
                                   std::uint64_t arena_charge)
    : kind_(kind), children_(std::move(children)), value_(std::move(value)),
      value_key_(std::move(value_key)), construction_key_(std::move(construction_key)),
      depth_(depth), arena_charge_(arena_charge)
{
}

ConstructionKind ConstructionNode::kind() const
{
    return kind_;
}
const std::vector<ConstructionNodeId>& ConstructionNode::children() const
{
    return children_;
}
const CanonicalReal& ConstructionNode::value() const
{
    return value_;
}
const std::vector<std::uint8_t>& ConstructionNode::value_key() const
{
    return value_key_.bytes();
}
const std::vector<std::uint8_t>& ConstructionNode::construction_key() const
{
    return construction_key_.bytes();
}
std::uint32_t ConstructionNode::depth() const
{
    return depth_;
}

ConstructionArena::ConstructionArena(Budget& budget) : budget_(budget) {}

ConstructionArena::~ConstructionArena()
{
    rollback(0);
}

std::size_t ConstructionArena::size() const
{
    return nodes_.size();
}

const ConstructionNode& ConstructionArena::at(ConstructionNodeId node) const
{
    return nodes_.at(node);
}

Budget& ConstructionArena::budget()
{
    return budget_;
}

bool ConstructionArena::valid_node(ConstructionNodeId node) const
{
    return static_cast<std::size_t>(node) < nodes_.size();
}

void ConstructionArena::rollback(std::size_t checkpoint)
{
    while (nodes_.size() > checkpoint)
    {
        const std::uint64_t charge = nodes_.back().arena_charge_;
        nodes_.pop_back();
        arena_storage_bytes_ -= charge;
        budget_.release_storage(charge);
    }
}

ConstructionResult ConstructionArena::intern_value(ConstructionKind kind,
                                                   std::vector<ConstructionNodeId> children,
                                                   CanonicalReal value)
{
    auto value_key = encode_canonical_real(budget_, value);
    if (value_key.error != Error::none || !value_key.value)
        return {value_key.error, std::nullopt};
    auto construction_key =
        make_construction_key(budget_, kind, children, nodes_, *value_key.value);
    if (construction_key.error != Error::none || !construction_key.value)
        return {construction_key.error, std::nullopt};
    try
    {
        std::uint64_t retained_key_bytes = 0;
        for (const ConstructionNode& node : nodes_)
            retained_key_bytes = checked_add(retained_key_bytes, node.construction_key().size());
        const std::uint64_t candidate_comparisons =
            checked_multiply(construction_key.value->bytes().size(), checked_add(nodes_.size(), 1));
        const std::uint64_t comparison_work =
            checked_add(checked_multiply(4, checked_add(retained_key_bytes, candidate_comparisons)),
                        checked_multiply(64, checked_add(nodes_.size(), 1)));
        if (!budget_.consume_work(comparison_work))
            return {Error::resource_limit_exceeded, std::nullopt};
        for (std::size_t index = 0; index < nodes_.size(); ++index)
            if (nodes_[index].construction_key() == construction_key.value->bytes())
                return {Error::none, static_cast<ConstructionNodeId>(index)};
        if (nodes_.size() >= std::numeric_limits<ConstructionNodeId>::max())
            return {Error::resource_limit_exceeded, std::nullopt};
        const std::uint64_t arena_charge = checked_add(512, checked_multiply(children.size(), 16));
        const std::uint64_t new_arena_storage = checked_add(arena_storage_bytes_, arena_charge);
        StorageReservation reservation(budget_, arena_charge);
        if (!reservation.acquired())
            return {Error::resource_limit_exceeded, std::nullopt};
        std::uint32_t depth = 0;
        for (const ConstructionNodeId child : children)
        {
            if (nodes_[child].depth() == std::numeric_limits<std::uint32_t>::max())
                return {Error::resource_limit_exceeded, std::nullopt};
            depth = std::max(depth, static_cast<std::uint32_t>(nodes_[child].depth() + 1));
        }
        const auto id = static_cast<ConstructionNodeId>(nodes_.size());
        ConstructionNode node(kind, std::move(children), std::move(value),
                              std::move(*value_key.value), std::move(*construction_key.value),
                              depth, arena_charge);
        nodes_.push_back(std::move(node));
        arena_storage_bytes_ = new_arena_storage;
        reservation.commit();
        return {Error::none, id};
    }
    catch (const std::exception&)
    {
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

ConstructionResult ConstructionArena::make_rational(const BigInt& numerator,
                                                    const BigInt& denominator)
{
    const std::size_t checkpoint = nodes_.size();
    auto value = make_canonical_rational(budget_, numerator, denominator);
    if (value.error != Error::none || !value.value)
        return {value.error, std::nullopt};
    ConstructionResult result =
        intern_value(ConstructionKind::rational, {}, std::move(*value.value));
    if (result.error != Error::none)
        rollback(checkpoint);
    return result;
}

ConstructionResult ConstructionArena::make_sum(const std::vector<ConstructionNodeId>& children)
{
    return make_associative(ConstructionKind::sum, children);
}

ConstructionResult ConstructionArena::make_product(const std::vector<ConstructionNodeId>& children)
{
    return make_associative(ConstructionKind::product, children);
}

ConstructionResult
ConstructionArena::make_associative(ConstructionKind kind,
                                    const std::vector<ConstructionNodeId>& input_children)
{
    const std::size_t checkpoint = nodes_.size();
    if (kind != ConstructionKind::sum && kind != ConstructionKind::product)
        return {Error::invalid_argument, std::nullopt};
    for (const ConstructionNodeId child : input_children)
        if (!valid_node(child))
            return {Error::invalid_argument, std::nullopt};
    try
    {
        std::uint64_t flattened_count = 0;
        std::uint64_t key_bytes = 0;
        for (const ConstructionNodeId child : input_children)
        {
            const ConstructionNode& node = nodes_[child];
            const std::uint64_t contribution = node.kind() == kind ? node.children().size() : 1;
            flattened_count = checked_add(flattened_count, contribution);
            key_bytes = checked_add(key_bytes, node.construction_key().size());
        }
        const std::uint64_t work = checked_add(
            checked_multiply(64, checked_add(flattened_count, 1)),
            checked_multiply(checked_add(key_bytes, 1), checked_add(flattened_count, 1)));
        const std::uint64_t storage = checked_add(4096, checked_multiply(flattened_count, 128));
        if (flattened_count > std::numeric_limits<std::size_t>::max())
            return {Error::resource_limit_exceeded, std::nullopt};
        StorageReservation reservation(budget_, storage);
        if (!reservation.acquired() || !budget_.consume_work(work))
            return {Error::resource_limit_exceeded, std::nullopt};
        std::vector<ConstructionNodeId> flattened;
        flattened.reserve(static_cast<std::size_t>(flattened_count));
        for (const ConstructionNodeId child : input_children)
        {
            const ConstructionNode& node = nodes_[child];
            if (node.kind() == kind)
                flattened.insert(flattened.end(), node.children().begin(), node.children().end());
            else
                flattened.push_back(child);
        }

        auto folded = make_canonical_rational(budget_, kind == ConstructionKind::sum ? 0 : 1, 1);
        if (folded.error != Error::none || !folded.value)
            return {folded.error, std::nullopt};
        std::vector<ConstructionNodeId> rational_children;
        rational_children.reserve(flattened.size());
        std::vector<ConstructionNodeId> nonrational;
        nonrational.reserve(flattened.size());
        for (const ConstructionNodeId child : flattened)
        {
            const CanonicalReal& value = nodes_[child].value();
            if (value.kind() == CanonicalRealKind::rational)
                rational_children.push_back(child);
            else
                nonrational.push_back(child);
        }
        const auto ordered_by_key = [&](ConstructionNodeId left, ConstructionNodeId right)
        { return key_less(nodes_[left], nodes_[right]); };
        std::sort(rational_children.begin(), rational_children.end(), ordered_by_key);
        std::sort(nonrational.begin(), nonrational.end(), ordered_by_key);
        for (const ConstructionNodeId child : rational_children)
        {
            const CanonicalReal& value = nodes_[child].value();
            CanonicalRealResult next =
                kind == ConstructionKind::sum
                    ? add_canonical_reals(budget_, *folded.value, value)
                    : multiply_canonical_reals(budget_, *folded.value, value);
            if (next.error != Error::none || !next.value)
                return {next.error, std::nullopt};
            folded.value = std::move(next.value);
            if (kind == ConstructionKind::product && folded.value->numerator() == 0)
            {
                ConstructionResult zero =
                    intern_value(ConstructionKind::rational, {}, std::move(*folded.value));
                if (zero.error != Error::none)
                    rollback(checkpoint);
                return zero;
            }
        }

        const bool keep_folded = !is_identity(*folded.value, kind);
        if (nonrational.empty())
        {
            ConstructionResult result =
                intern_value(ConstructionKind::rational, {}, std::move(*folded.value));
            if (result.error != Error::none)
                rollback(checkpoint);
            return result;
        }
        if (!keep_folded && nonrational.size() == 1)
            return {Error::none, nonrational.front()};

        if (keep_folded)
        {
            ConstructionResult rational =
                intern_value(ConstructionKind::rational, {}, std::move(*folded.value));
            if (rational.error != Error::none || !rational.node)
            {
                rollback(checkpoint);
                return rational;
            }
            nonrational.push_back(*rational.node);
        }
        std::sort(nonrational.begin(), nonrational.end(),
                  [&](ConstructionNodeId left, ConstructionNodeId right)
                  { return key_less(nodes_[left], nodes_[right]); });
        CanonicalRealResult evaluated =
            make_canonical_rational(budget_, kind == ConstructionKind::sum ? 0 : 1, 1);
        if (evaluated.error != Error::none || !evaluated.value)
        {
            rollback(checkpoint);
            return {evaluated.error, std::nullopt};
        }
        for (const ConstructionNodeId child : nonrational)
        {
            CanonicalRealResult next =
                kind == ConstructionKind::sum
                    ? add_canonical_reals(budget_, *evaluated.value, nodes_[child].value())
                    : multiply_canonical_reals(budget_, *evaluated.value, nodes_[child].value());
            if (next.error != Error::none || !next.value)
            {
                rollback(checkpoint);
                return {next.error, std::nullopt};
            }
            evaluated.value = std::move(next.value);
        }
        if (evaluated.value->kind() == CanonicalRealKind::rational)
        {
            rollback(checkpoint);
            ConstructionResult result =
                intern_value(ConstructionKind::rational, {}, std::move(*evaluated.value));
            if (result.error != Error::none)
                rollback(checkpoint);
            return result;
        }
        ConstructionResult result =
            intern_value(kind, std::move(nonrational), std::move(*evaluated.value));
        if (result.error != Error::none)
            rollback(checkpoint);
        return result;
    }
    catch (const std::exception&)
    {
        rollback(checkpoint);
        return {Error::resource_limit_exceeded, std::nullopt};
    }
}

ConstructionResult ConstructionArena::make_reciprocal(ConstructionNodeId child)
{
    const std::size_t checkpoint = nodes_.size();
    if (!valid_node(child))
        return {Error::invalid_argument, std::nullopt};
    CanonicalRealResult value = reciprocal_canonical_real(budget_, nodes_[child].value());
    if (value.error != Error::none || !value.value)
        return {value.error, std::nullopt};
    const ConstructionKind kind = value.value->kind() == CanonicalRealKind::rational
                                      ? ConstructionKind::rational
                                      : ConstructionKind::reciprocal;
    StorageReservation reservation(budget_, 512);
    if (!reservation.acquired() || !budget_.consume_work(64))
        return {Error::resource_limit_exceeded, std::nullopt};
    std::vector<ConstructionNodeId> children;
    if (kind != ConstructionKind::rational)
        children.push_back(child);
    ConstructionResult result = intern_value(kind, std::move(children), std::move(*value.value));
    if (result.error != Error::none)
        rollback(checkpoint);
    return result;
}

ConstructionResult ConstructionArena::make_nonnegative_square_root(ConstructionNodeId child)
{
    const std::size_t checkpoint = nodes_.size();
    if (!valid_node(child))
        return {Error::invalid_argument, std::nullopt};
    CanonicalRealResult value =
        nonnegative_square_root_canonical_real(budget_, nodes_[child].value());
    if (value.error != Error::none || !value.value)
        return {value.error, std::nullopt};
    const ConstructionKind kind = value.value->kind() == CanonicalRealKind::rational
                                      ? ConstructionKind::rational
                                      : ConstructionKind::nonnegative_square_root;
    StorageReservation reservation(budget_, 512);
    if (!reservation.acquired() || !budget_.consume_work(64))
        return {Error::resource_limit_exceeded, std::nullopt};
    std::vector<ConstructionNodeId> children;
    if (kind != ConstructionKind::rational)
        children.push_back(child);
    ConstructionResult result = intern_value(kind, std::move(children), std::move(*value.value));
    if (result.error != Error::none)
        rollback(checkpoint);
    return result;
}

} // namespace geometer::exact
