#include "geometer/ipc_a0_frame.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{

using geometer::ipc_a0::Attachment;
using geometer::ipc_a0::Frame;
using geometer::ipc_a0::FrameKind;
using geometer::ipc_a0::ReadStatus;

void require(bool condition)
{
    if (!condition)
    {
        std::abort();
    }
}

std::FILE* temporary_file()
{
    std::FILE* file = nullptr;
#ifdef _WIN32
    require(tmpfile_s(&file) == 0);
#else
    file = std::tmpfile();
#endif
    require(file != nullptr);
    return file;
}

void exact_shutdown_vector()
{
    std::FILE* file = temporary_file();
    const Frame frame{FrameKind::shutdown, 0, "{}", {}};
    std::string error;
    require(geometer::ipc_a0::write_frame(file, frame, &error));
    require(error.empty());
    require(std::fseek(file, 0, SEEK_SET) == 0);
    std::array<unsigned char, 50> bytes{};
    require(std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size());
    const std::array<unsigned char, 50> expected = {
        0x47, 0x4d, 0x49, 0x50, 0x43, 0x41, 0x30, 0x31, 0x30, 0x00, 0x00, 0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x7d,
    };
    require(bytes == expected);
    std::fclose(file);
}

void binary_attachment_round_trip()
{
    std::FILE* file = temporary_file();
    Frame frame{FrameKind::request, 42, "{\"operation\":\"synthetic\",\"request\":{}}", {}};
    frame.attachments.push_back(
        Attachment{"model", "application/octet-stream", {0x00, 0xff, 0x0a, 0x00}});
    std::string error;
    require(geometer::ipc_a0::write_frame(file, frame, &error));
    require(std::fseek(file, 0, SEEK_SET) == 0);
    Frame decoded;
    require(geometer::ipc_a0::read_frame(file, &decoded, &error) == ReadStatus::ok);
    require(decoded.kind == frame.kind);
    require(decoded.request_id == frame.request_id);
    require(decoded.json == frame.json);
    require(decoded.attachments.size() == 1);
    require(decoded.attachments.front().data == frame.attachments.front().data);
    std::fclose(file);
}

void oversized_fixed_header_fails_without_payload()
{
    std::FILE* file = temporary_file();
    std::array<unsigned char, geometer::ipc_a0::kHeaderSize> header{};
    const std::array<unsigned char, 8> magic = {'G', 'M', 'I', 'P', 'C', 'A', '0', '1'};
    std::copy(magic.begin(), magic.end(), header.begin());
    header[8] = 48;
    header[12] = static_cast<unsigned char>(FrameKind::request);
    header[16] = 1;
    header[24] = 1;
    header[35] = 0x40; // 1 GiB attachment section.
    require(std::fwrite(header.data(), 1, header.size(), file) == header.size());
    require(std::fseek(file, 0, SEEK_SET) == 0);
    Frame decoded;
    std::string error;
    require(geometer::ipc_a0::read_frame(file, &decoded, &error) == ReadStatus::error);
    require(error.find("limit") != std::string::npos);
    std::fclose(file);
}

void attachment_subsection_cannot_exceed_declared_payload()
{
    std::FILE* file = temporary_file();
    std::array<unsigned char, geometer::ipc_a0::kHeaderSize + 2U + 16U> bytes{};
    const std::array<unsigned char, 8> magic = {'G', 'M', 'I', 'P', 'C', 'A', '0', '1'};
    std::copy(magic.begin(), magic.end(), bytes.begin());
    bytes[8] = 48;
    bytes[12] = static_cast<unsigned char>(FrameKind::request);
    bytes[16] = 1;
    bytes[24] = 2;
    bytes[28] = 1;
    bytes[32] = 16;
    bytes[48] = '{';
    bytes[49] = '}';
    bytes[50 + 8] = 100; // Attachment data is larger than its containing section.
    require(std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size());
    require(std::fseek(file, 0, SEEK_SET) == 0);
    Frame decoded;
    std::string error;
    require(geometer::ipc_a0::read_frame(file, &decoded, &error) == ReadStatus::error);
    require(error.find("boundary") != std::string::npos);
    std::fclose(file);
}

void invalid_attachment_metadata_is_rejected()
{
    std::FILE* file = temporary_file();
    Frame frame{FrameKind::request, 42, "{}", {}};
    frame.attachments.push_back(Attachment{"model", "application/step", {0x01}});
    frame.attachments.push_back(Attachment{"model", "application/step", {0x02}});
    std::string error;
    require(!geometer::ipc_a0::write_frame(file, frame, &error));
    require(error.find("limit") != std::string::npos);
    frame.attachments.resize(1);
    frame.attachments.front().name = std::string("\xc0\x80", 2);
    require(!geometer::ipc_a0::write_frame(file, frame, &error));
    require(error.find("limit") != std::string::npos);
    std::fclose(file);
}

} // namespace

int main()
{
    exact_shutdown_vector();
    binary_attachment_round_trip();
    oversized_fixed_header_fails_without_payload();
    attachment_subsection_cannot_exceed_declared_payload();
    invalid_attachment_metadata_is_rejected();
    return 0;
}
