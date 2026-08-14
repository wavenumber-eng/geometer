#include "geometer/exact_artifact.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using geometer::exact::ConstructionArena;
using geometer::exact::ConstructionNodeId;
using geometer::exact::Error;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

ConstructionNodeId require_node(const geometer::exact::ConstructionResult& result,
                                const std::string& message)
{
    require(result.error == Error::none && result.node.has_value(), message);
    return *result.node;
}

ConstructionNodeId make_square_root(ConstructionArena& arena, int radicand)
{
    const ConstructionNodeId rational =
        require_node(arena.make_rational(radicand), "artifact radicand failed");
    return require_node(arena.make_nonnegative_square_root(rational),
                        "artifact square root failed");
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) |
           static_cast<std::uint32_t>(bytes[offset + 1]) << 8 |
           static_cast<std::uint32_t>(bytes[offset + 2]) << 16 |
           static_cast<std::uint32_t>(bytes[offset + 3]) << 24;
}

void write_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
{
    for (std::uint32_t shift = 0; shift < 32; shift += 8)
        bytes[offset++] = static_cast<std::uint8_t>((value >> shift) & 0xff);
}

std::vector<std::uint8_t> zero_golden()
{
    return {
        71, 69, 88, 80, 65, 48, 48, 49, 1, 0,  0, 0, 1, 0, 0,  0, 1, 0, 0, 0, 0, 0,  0,
        0,  92, 0,  0,  0,  0,  0,  0,  0, 56, 0, 0, 0, 1, 0,  0, 0, 0, 0, 0, 0, 32, 0,
        0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  1, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0,  0,
        0,  0,  0,  1,  0,  0,  0,  1,  0, 0,  0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0,  0,
    };
}

void test_zero_golden_and_budget_boundaries()
{
    geometer::exact::Budget fixture_budget({1'000'000'000, 268'435'456});
    ConstructionArena arena(fixture_budget);
    const ConstructionNodeId zero = require_node(arena.make_rational(0), "zero node failed");

    geometer::exact::Budget encode_budget({984, 1'000'000});
    auto encoded = geometer::exact::encode_construction_artifact(encode_budget, arena, {zero});
    const auto expected = zero_golden();
    require(encoded.error == Error::none && encoded.value && encoded.value->bytes() == expected &&
                encode_budget.usage().work_units == 984,
            "zero GEXPA001 golden or exact encode boundary changed");
    encoded.value.reset();
    require(encode_budget.usage().owned_bytes == 0,
            "encoded artifact ownership must release deterministically");

    geometer::exact::Budget short_encode({983, 1'000'000});
    auto short_result = geometer::exact::encode_construction_artifact(short_encode, arena, {zero});
    require(short_result.error == Error::resource_limit_exceeded && !short_result.value &&
                short_encode.usage().work_units == 248 && short_encode.usage().owned_bytes == 0,
            "one-unit-short artifact encode must fail after planning and roll storage back");

    geometer::exact::Budget decode_budget({1'000'000'000, 268'435'456});
    auto decoded = geometer::exact::decode_construction_artifact(decode_budget, expected);
    require(decoded.error == Error::none && decoded.value && decoded.value->arena().size() == 1 &&
                decoded.value->roots() == std::vector<ConstructionNodeId>({0}),
            "zero artifact strict decode failed");
    auto reencoded = geometer::exact::encode_construction_artifact(
        decode_budget, decoded.value->arena(), decoded.value->roots());
    require(reencoded.error == Error::none && reencoded.value &&
                reencoded.value->bytes() == expected,
            "decoded zero artifact failed canonical byte replay");

    geometer::exact::Budget exact_decode({2'980, 1'000'000});
    auto exact_decoded = geometer::exact::decode_construction_artifact(exact_decode, expected);
    require(exact_decoded.error == Error::none && exact_decoded.value &&
                exact_decode.usage().work_units == 2'980,
            "zero artifact exact decode work boundary changed");
    exact_decoded.value.reset();
    require(exact_decode.usage().owned_bytes == 0,
            "decoded artifact ownership must release its arena and root storage");

    geometer::exact::Budget short_decode({2'979, 1'000'000});
    auto short_decoded = geometer::exact::decode_construction_artifact(short_decode, expected);
    require(short_decoded.error == Error::resource_limit_exceeded && !short_decoded.value &&
                short_decode.usage().work_units == 2'812 && short_decode.usage().owned_bytes == 0,
            "one-unit-short artifact decode must retain work and roll semantic storage back");
}

void test_order_independent_canonical_artifact_and_reachability()
{
    geometer::exact::Budget left_budget({1'000'000'000, 268'435'456});
    ConstructionArena left(left_budget);
    const ConstructionNodeId left_two = make_square_root(left, 2);
    const ConstructionNodeId left_three = make_square_root(left, 3);
    const ConstructionNodeId left_sum =
        require_node(left.make_sum({left_two, left_three}), "left artifact sum failed");

    geometer::exact::Budget right_budget({1'000'000'000, 268'435'456});
    ConstructionArena right(right_budget);
    const ConstructionNodeId unused = require_node(right.make_rational(99), "unused node failed");
    const ConstructionNodeId right_three = make_square_root(right, 3);
    const ConstructionNodeId right_two = make_square_root(right, 2);
    const ConstructionNodeId right_sum =
        require_node(right.make_sum({right_three, right_two}), "right artifact sum failed");
    require(unused != right_sum, "unused fixture unexpectedly interned with sum");

    geometer::exact::Budget left_encode({1'000'000'000, 268'435'456});
    geometer::exact::Budget right_encode({1'000'000'000, 268'435'456});
    auto left_bytes =
        geometer::exact::encode_construction_artifact(left_encode, left, {left_sum, left_two});
    auto right_bytes =
        geometer::exact::encode_construction_artifact(right_encode, right, {right_sum, right_two});
    require(left_bytes.value && right_bytes.value &&
                left_bytes.value->bytes() == right_bytes.value->bytes(),
            "allocation order and unreachable nodes must not alter canonical artifact bytes");

    geometer::exact::Budget decode_budget({1'000'000'000, 268'435'456});
    auto decoded =
        geometer::exact::decode_construction_artifact(decode_budget, left_bytes.value->bytes());
    require(decoded.error == Error::none && decoded.value && decoded.value->roots().size() == 2 &&
                decoded.value->arena().size() == 5 &&
                decoded.value->arena().at(decoded.value->roots()[0]).value_key() ==
                    left.at(left_sum).value_key() &&
                decoded.value->arena().at(decoded.value->roots()[1]).value_key() ==
                    left.at(left_two).value_key(),
            "multi-node canonical artifact round trip failed");

    geometer::exact::Budget reversed_budget({1'000'000'000, 268'435'456});
    auto reversed =
        geometer::exact::encode_construction_artifact(reversed_budget, left, {left_two, left_sum});
    require(reversed.value && reversed.value->bytes() != left_bytes.value->bytes() &&
                std::equal(reversed.value->bytes().begin(), reversed.value->bytes().end() - 8,
                           left_bytes.value->bytes().begin()),
            "semantic root order must be preserved only in the ordered root table");
}

void test_strict_structural_rejection()
{
    const auto golden = zero_golden();
    auto expect_invalid = [&](std::vector<std::uint8_t> candidate, const std::string& message)
    {
        geometer::exact::Budget budget({1'000'000'000, 268'435'456});
        auto result = geometer::exact::decode_construction_artifact(budget, candidate);
        require(result.error == Error::invalid_argument && !result.value, message);
    };

    auto magic = golden;
    magic[0] = 'B';
    expect_invalid(std::move(magic), "invalid artifact magic was accepted");
    auto generation = golden;
    generation[8] = 2;
    expect_invalid(std::move(generation), "unknown artifact generation was accepted");
    auto flags = golden;
    flags[10] = 1;
    expect_invalid(std::move(flags), "nonzero artifact flags were accepted");
    auto total = golden;
    total[24] = 91;
    expect_invalid(std::move(total), "incorrect artifact total bytes were accepted");
    auto record_size = golden;
    record_size[32] = 64;
    expect_invalid(std::move(record_size), "nonminimal node record length was accepted");
    auto kind = golden;
    kind[36] = 9;
    expect_invalid(std::move(kind), "unknown construction kind was accepted");
    auto node_reserved = golden;
    node_reserved[37] = 1;
    expect_invalid(std::move(node_reserved), "nonzero node flags were accepted");
    auto scalar = golden;
    scalar[64] = 1;
    expect_invalid(std::move(scalar), "noncanonical embedded scalar was accepted");
    auto root = golden;
    root[88] = 1;
    expect_invalid(std::move(root), "out-of-range artifact root was accepted");
}

void test_duplicate_unreachable_order_and_padding_rejection()
{
    geometer::exact::Budget rational_budget({1'000'000'000, 268'435'456});
    ConstructionArena rationals(rational_budget);
    const ConstructionNodeId zero = require_node(rationals.make_rational(0), "zero failed");
    const ConstructionNodeId one = require_node(rationals.make_rational(1), "one failed");
    geometer::exact::Budget encode_budget({1'000'000'000, 268'435'456});
    auto encoded =
        geometer::exact::encode_construction_artifact(encode_budget, rationals, {zero, one});
    require(encoded.value && read_u32(encoded.value->bytes(), 12) == 2,
            "two-rational artifact setup failed");
    const auto bytes = encoded.value->bytes();
    auto expect_invalid = [&](std::vector<std::uint8_t> candidate, const std::string& message)
    {
        geometer::exact::Budget budget({1'000'000'000, 268'435'456});
        auto result = geometer::exact::decode_construction_artifact(budget, candidate);
        require(result.error == Error::invalid_argument && !result.value, message);
    };
    auto unreachable = bytes;
    write_u32(unreachable, unreachable.size() - 4, read_u32(unreachable, unreachable.size() - 8));
    expect_invalid(std::move(unreachable), "unreachable encoded node was accepted");
    auto duplicate = bytes;
    std::copy(duplicate.begin() + 32, duplicate.begin() + 88, duplicate.begin() + 88);
    expect_invalid(std::move(duplicate), "duplicate construction key was accepted");
    auto unordered = bytes;
    std::vector<std::uint8_t> first(unordered.begin() + 32, unordered.begin() + 88);
    std::copy(unordered.begin() + 88, unordered.begin() + 144, unordered.begin() + 32);
    std::copy(first.begin(), first.end(), unordered.begin() + 88);
    expect_invalid(std::move(unordered), "noncanonical dependency-depth/key order was accepted");

    geometer::exact::Budget root_budget({1'000'000'000, 268'435'456});
    ConstructionArena root_arena(root_budget);
    const ConstructionNodeId sqrt_two = make_square_root(root_arena, 2);
    geometer::exact::Budget root_encode({1'000'000'000, 268'435'456});
    auto root_bytes =
        geometer::exact::encode_construction_artifact(root_encode, root_arena, {sqrt_two});
    require(root_bytes.value.has_value(), "unary artifact setup failed");
    auto forward_child = root_bytes.value->bytes();
    const std::size_t second_record = 32 + read_u32(forward_child, 32);
    write_u32(forward_child, second_record + 24, 1);
    expect_invalid(std::move(forward_child), "forward child index was accepted");
    auto padding = root_bytes.value->bytes();
    const std::uint32_t second_bytes = read_u32(padding, second_record);
    padding[second_record + second_bytes - 1] = 1;
    expect_invalid(std::move(padding), "nonzero node alignment padding was accepted");
}

} // namespace

int main()
{
    test_zero_golden_and_budget_boundaries();
    test_order_independent_canonical_artifact_and_reachability();
    test_strict_structural_rejection();
    test_duplicate_unreachable_order_and_padding_rejection();
    return 0;
}
