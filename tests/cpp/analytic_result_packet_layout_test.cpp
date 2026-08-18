#include "geometer/analytic_result_packet_layout.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace
{

using namespace geometer;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void write_u64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value)
{
    for (std::uint32_t index = 0; index < 8; ++index)
        bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
}

std::string hex(const std::vector<std::uint8_t>& bytes)
{
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::uint8_t byte : bytes)
        out << std::setw(2) << static_cast<unsigned>(byte);
    return out.str();
}

void require_invalid(const std::vector<std::uint8_t>& bytes, const std::string& message)
{
    AnalyticResultPacketLayoutResult result =
        decode_analytic_result_packet_layout(bytes.data(), bytes.size());
    require(result.error == AnalyticResultPacketLayoutError::invalid_packet && !result.value,
            message);
}

void require_limit(const std::vector<std::uint8_t>& bytes, const std::string& message)
{
    AnalyticResultPacketLayoutResult result =
        decode_analytic_result_packet_layout(bytes.data(), bytes.size());
    require(result.error == AnalyticResultPacketLayoutError::limit_exceeded && !result.value,
            message);
}

} // namespace

int main()
{
    AnalyticResultTableBytes empty_tables;
    AnalyticResultPacketEncodeResult empty = encode_analytic_result_packet_layout(empty_tables);
    require(empty.error == AnalyticResultPacketLayoutError::none && empty.value &&
                empty.value->size() == 512,
            "unique empty result packet encoding failed");
    AnalyticResultPacketLayoutResult decoded_empty =
        decode_analytic_result_packet_layout(empty.value->data(), empty.value->size());
    require(decoded_empty.error == AnalyticResultPacketLayoutError::none && decoded_empty.value &&
                decoded_empty.value->canonical_payload_bytes == 0 &&
                decoded_empty.value->job_result_count == 0 &&
                decoded_empty.value->relationship_result_count == 0,
            "unique empty result packet decoding failed");
    for (const AnalyticResultTableView& table : decoded_empty.value->tables)
        require(table.offset == 512 && table.byte_length == 0 && table.record_count == 0,
                "empty table offset is not canonical");

    constexpr std::uint32_t record_bytes[]{48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4};
    AnalyticResultTableBytes all_tables;
    for (std::size_t index = 0; index < kAnalyticResultTableCount; ++index)
        all_tables[index].assign(record_bytes[index], static_cast<std::uint8_t>(index + 1));
    AnalyticResultPacketEncodeResult encoded = encode_analytic_result_packet_layout(all_tables);
    require(encoded.error == AnalyticResultPacketLayoutError::none && encoded.value &&
                encoded.value->size() == 924,
            "all-table result layout encoding failed");
    AnalyticResultPacketLayoutResult decoded =
        decode_analytic_result_packet_layout(encoded.value->data(), encoded.value->size());
    require(decoded.error == AnalyticResultPacketLayoutError::none && decoded.value &&
                decoded.value->job_result_count == 1 &&
                decoded.value->relationship_result_count == 1 &&
                decoded.value->canonical_payload_bytes == 408,
            "all-table result layout decoding failed");
    require(decoded.value->tables[5].offset == 728 && decoded.value->tables[5].byte_length == 4 &&
                decoded.value->tables[6].offset == 736 && (*encoded.value)[732] == 0 &&
                (*encoded.value)[733] == 0 && (*encoded.value)[734] == 0 &&
                (*encoded.value)[735] == 0,
            "odd 4-byte table did not use minimum zero alignment padding");

    std::vector<std::uint8_t> malformed = *encoded.value;
    malformed[0] = 0;
    require_invalid(malformed, "bad result magic was accepted");
    malformed = *encoded.value;
    malformed[8] = 2;
    require_invalid(malformed, "bad result generation was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 16, malformed.size() - 1);
    require_invalid(malformed, "bad total packet length was accepted");
    malformed = *encoded.value;
    malformed[64] = 102;
    require_invalid(malformed, "bad result table kind was accepted");
    malformed = *encoded.value;
    malformed[66] = 2;
    require_invalid(malformed, "bad result record version was accepted");
    malformed = *encoded.value;
    malformed[68] = 47;
    require_invalid(malformed, "bad fixed record size was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 72, 513);
    require_invalid(malformed, "misaligned/noncanonical table offset was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 80, 47);
    require_invalid(malformed, "bad table byte length was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 88, 2);
    require_invalid(malformed, "record-count multiplication mismatch was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 88, std::numeric_limits<std::uint64_t>::max());
    require_limit(malformed, "record-count multiplication overflow was not bounded");
    malformed = *encoded.value;
    malformed[732] = 1;
    require_invalid(malformed, "nonzero minimum-alignment padding was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 48, 407);
    require_invalid(malformed, "bad canonical payload sum was accepted");
    malformed = *encoded.value;
    malformed[36] = 0;
    require_invalid(malformed, "bad header job count was accepted");
    malformed = *encoded.value;
    malformed[36] = 0;
    malformed[37] = 0;
    malformed[38] = 1;
    malformed[39] = 0;
    require_limit(malformed, "job-result limit was not enforced before table traversal");
    malformed = *encoded.value;
    malformed.pop_back();
    require_invalid(malformed, "truncated result packet was accepted");
    malformed = *encoded.value;
    malformed.push_back(0);
    require_invalid(malformed, "trailing result bytes were accepted");

    AnalyticResultTableBytes invalid_tables;
    invalid_tables[0].push_back(0);
    AnalyticResultPacketEncodeResult invalid = encode_analytic_result_packet_layout(invalid_tables);
    require(invalid.error == AnalyticResultPacketLayoutError::invalid_packet && !invalid.value,
            "partial fixed record was encoded");
    const std::uint8_t byte = 0;
    AnalyticResultPacketLayoutResult oversized =
        decode_analytic_result_packet_layout(&byte, 268'435'457);
    require(oversized.error == AnalyticResultPacketLayoutError::limit_exceeded && !oversized.value,
            "oversized result packet did not fail before reading memory");

    std::cout << "ANALYTIC_RESULT_PACKET_EMPTY=" << hex(*empty.value) << '\n';
    std::cout << "ANALYTIC_RESULT_PACKET_LAYOUT_VECTOR=" << hex(*encoded.value) << '\n';
    return 0;
}
