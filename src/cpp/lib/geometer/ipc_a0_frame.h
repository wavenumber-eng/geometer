#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace geometer::ipc_a0
{

constexpr std::size_t kHeaderSize = 48U;
constexpr std::size_t kMaxJsonBytes = 8U * 1024U * 1024U;
constexpr std::size_t kMaxAttachmentCount = 16U;
constexpr std::size_t kMaxAttachmentTextBytes = 128U;
constexpr std::size_t kMaxAttachmentBytes = 256U * 1024U * 1024U;
constexpr std::size_t kMaxFrameBytes = 512U * 1024U * 1024U;

enum class FrameKind : std::uint16_t
{
    hello = 1,
    welcome = 2,
    request = 3,
    response = 4,
    cancel = 5,
    cancelled = 6,
    cancel_rejected = 7,
    shutdown = 8,
    shutdown_ack = 9,
    protocol_error = 10,
};

struct Attachment
{
    std::string name;
    std::string media_type;
    std::vector<unsigned char> data;
};

struct Frame
{
    FrameKind kind{};
    std::uint64_t request_id = 0;
    std::string json;
    std::vector<Attachment> attachments;
};

enum class ReadStatus
{
    ok,
    end_of_stream,
    error,
};

std::size_t encoded_size(const Frame& frame);
ReadStatus read_frame(std::FILE* stream, Frame* frame, std::string* error);
bool write_frame(std::FILE* stream, const Frame& frame, std::string* error);

} // namespace geometer::ipc_a0
