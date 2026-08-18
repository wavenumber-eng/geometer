#pragma once

#include "geometer/exact_expression.h"
#include "geometer/exact_value_codec.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

using ConstructionNodeId = std::uint32_t;

enum class ConstructionKind : std::uint8_t
{
    rational = 1,
    sum = 2,
    product = 3,
    reciprocal = 4,
    nonnegative_square_root = 5,
};

class ConstructionNode
{
  public:
    ConstructionNode(ConstructionNode&& other) noexcept = default;
    ConstructionNode& operator=(ConstructionNode&& other) noexcept = default;
    ConstructionNode(const ConstructionNode&) = delete;
    ConstructionNode& operator=(const ConstructionNode&) = delete;

    [[nodiscard]] ConstructionKind kind() const;
    [[nodiscard]] const std::vector<ConstructionNodeId>& children() const;
    [[nodiscard]] const CanonicalReal& value() const;
    [[nodiscard]] const std::vector<std::uint8_t>& value_key() const;
    [[nodiscard]] const std::vector<std::uint8_t>& construction_key() const;
    [[nodiscard]] std::uint32_t depth() const;

  private:
    friend class ConstructionArena;
    ConstructionNode(ConstructionKind kind, std::vector<ConstructionNodeId> children,
                     CanonicalReal value, EncodedBytes value_key, EncodedBytes construction_key,
                     std::uint32_t depth, std::uint64_t arena_charge);

    ConstructionKind kind_ = ConstructionKind::rational;
    std::vector<ConstructionNodeId> children_;
    CanonicalReal value_;
    EncodedBytes value_key_;
    EncodedBytes construction_key_;
    std::uint32_t depth_ = 0;
    std::uint64_t arena_charge_ = 0;
};

struct ConstructionResult
{
    Error error = Error::none;
    std::optional<ConstructionNodeId> node;
};

class ConstructionArena
{
  public:
    explicit ConstructionArena(Budget& budget);
    ~ConstructionArena();
    ConstructionArena(const ConstructionArena&) = delete;
    ConstructionArena& operator=(const ConstructionArena&) = delete;

    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] const ConstructionNode& at(ConstructionNodeId node) const;
    [[nodiscard]] Budget& budget();

    [[nodiscard]] ConstructionResult make_rational(const BigInt& numerator,
                                                   const BigInt& denominator = 1);
    [[nodiscard]] ConstructionResult make_sum(const std::vector<ConstructionNodeId>& children);
    [[nodiscard]] ConstructionResult make_sum(ConstructionNodeId left, ConstructionNodeId right);
    [[nodiscard]] ConstructionResult make_product(const std::vector<ConstructionNodeId>& children);
    [[nodiscard]] ConstructionResult make_product(ConstructionNodeId left,
                                                  ConstructionNodeId right);
    [[nodiscard]] ConstructionResult make_reciprocal(ConstructionNodeId child);
    [[nodiscard]] ConstructionResult make_nonnegative_square_root(ConstructionNodeId child);

  private:
    friend class ConstructionArenaTransaction;
    [[nodiscard]] ConstructionResult
    make_associative(ConstructionKind kind, const std::vector<ConstructionNodeId>& input_children);
    [[nodiscard]] ConstructionResult make_binary(ConstructionKind kind, ConstructionNodeId left,
                                                 ConstructionNodeId right);
    [[nodiscard]] ConstructionResult intern_value(ConstructionKind kind,
                                                  std::vector<ConstructionNodeId> children,
                                                  CanonicalReal value);
    [[nodiscard]] bool valid_node(ConstructionNodeId node) const;
    void rollback(std::size_t checkpoint);

    Budget& budget_;
    std::vector<ConstructionNode> nodes_;
    std::uint64_t arena_storage_bytes_ = 0;
};

class ConstructionArenaTransaction
{
  public:
    explicit ConstructionArenaTransaction(ConstructionArena& arena);
    ~ConstructionArenaTransaction();
    ConstructionArenaTransaction(const ConstructionArenaTransaction&) = delete;
    ConstructionArenaTransaction& operator=(const ConstructionArenaTransaction&) = delete;

    void commit();

  private:
    ConstructionArena& arena_;
    std::size_t checkpoint_ = 0;
    bool committed_ = false;
};

class ConstructionBuilder
{
  public:
    explicit ConstructionBuilder(ConstructionArena& arena);

    [[nodiscard]] ConstructionNodeId rational(const BigInt& numerator,
                                              const BigInt& denominator = 1);
    [[nodiscard]] ConstructionNodeId sum(ConstructionNodeId left, ConstructionNodeId right);
    [[nodiscard]] ConstructionNodeId product(ConstructionNodeId left, ConstructionNodeId right);
    [[nodiscard]] ConstructionNodeId negate(ConstructionNodeId value);
    [[nodiscard]] ConstructionNodeId subtract(ConstructionNodeId left, ConstructionNodeId right);
    [[nodiscard]] ConstructionNodeId square(ConstructionNodeId value);
    [[nodiscard]] ConstructionNodeId divide(ConstructionNodeId numerator,
                                            ConstructionNodeId denominator);
    [[nodiscard]] ConstructionNodeId square_root(ConstructionNodeId value);
    [[nodiscard]] std::int8_t sign(ConstructionNodeId value);
    [[nodiscard]] std::int8_t compare(ConstructionNodeId left, ConstructionNodeId right);
    [[nodiscard]] bool good() const;
    [[nodiscard]] Error error() const;

  private:
    [[nodiscard]] ConstructionNodeId take(ConstructionResult result);

    ConstructionArena& arena_;
    Error error_ = Error::none;
};

} // namespace geometer::exact
