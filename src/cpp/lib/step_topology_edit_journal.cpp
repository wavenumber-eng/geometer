#include "step_topology_session_internal.h"

#include "geometer/sha256.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace geometer::step_topology_internal
{
namespace
{

constexpr std::array<unsigned char, 8> kMagic = {'W', 'N', 'G', 'T', 'J', 'A', '0', 0};
constexpr std::uint32_t kVersion = 0;
constexpr std::size_t kDigestBytes = 32;
thread_local EditJournalReplayApplyEntryHook replay_apply_entry_hook = nullptr;
thread_local void* replay_apply_entry_context = nullptr;

bool checked_add(std::size_t* total, std::size_t value)
{
    if (*total > std::numeric_limits<std::size_t>::max() - value)
        return false;
    *total += value;
    return true;
}

bool checked_add_scaled(std::size_t* total, std::size_t value, std::size_t scale)
{
    if (value != 0 && scale > std::numeric_limits<std::size_t>::max() / value)
        return false;
    return checked_add(total, value * scale);
}

bool valid_sha256(const std::string& value)
{
    return value.size() == 64U && std::all_of(value.begin(), value.end(),
                                              [](unsigned char character)
                                              {
                                                  return (character >= '0' && character <= '9') ||
                                                         (character >= 'a' && character <= 'f');
                                              });
}

std::string number(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}

void hash_u64(Sha256Builder* hash, std::uint64_t value)
{
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = static_cast<std::uint8_t>(value >> (index * 8U));
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

void hash_label(Sha256Builder* hash, const StepTopologyLabelSummary& label)
{
    hash_u64(hash, label.present ? 1U : 0U);
    hash_string(hash, label.name);
    hash_u64(hash, label.color_assignments);
    hash_u64(hash, label.layer_assignments);
    hash_u64(hash, label.has_material_assignment ? 1U : 0U);
    hash_u64(hash, label.has_named_data ? 1U : 0U);
    hash_u64(hash, label.has_validation_properties ? 1U : 0U);
}

template <std::size_t Size>
void hash_doubles(Sha256Builder* hash, const std::array<double, Size>& values)
{
    for (double value : values)
        hash_double(hash, value);
}

template <std::size_t Size>
void append_values(std::ostringstream* stream, const std::array<double, Size>& values)
{
    for (double value : values)
        *stream << '|' << number(value);
}

std::string target_evidence(const StepTopologyBody& body, std::size_t definition_index)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "body|" << definition_index << '|' << body.topology_kind << '|'
           << body.shell_handles.size() << '|' << body.face_handles.size() << '|'
           << number(body.volume);
    append_values(&stream, body.bounds);
    const std::string encoded = stream.str();
    return sha256_hex(reinterpret_cast<const std::uint8_t*>(encoded.data()), encoded.size());
}

std::string target_evidence(const StepTopologyFace& face, std::size_t definition_index)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "face|" << definition_index << '|' << face.body_handles.size() << '|'
           << face.shell_handles.size() << '|' << number(face.area);
    append_values(&stream, face.bounds);
    for (double value : face.centroid)
        stream << '|' << number(value);
    const std::string encoded = stream.str();
    return sha256_hex(reinterpret_cast<const std::uint8_t*>(encoded.data()), encoded.size());
}

class Writer
{
  public:
    explicit Writer(std::size_t limit) : limit_(limit) {}

    bool append(const unsigned char* bytes, std::size_t size)
    {
        if (size > limit_ || bytes_.size() > limit_ - size)
            return false;
        bytes_.insert(bytes_.end(), bytes, bytes + size);
        return true;
    }

    bool u8(std::uint8_t value)
    {
        return append(&value, 1U);
    }

    bool u32(std::uint32_t value)
    {
        std::array<unsigned char, 4> encoded{};
        for (unsigned int shift = 0; shift < 32U; shift += 8U)
            encoded[shift / 8U] = static_cast<unsigned char>((value >> shift) & 0xffU);
        return append(encoded.data(), encoded.size());
    }

    bool u64(std::uint64_t value)
    {
        std::array<unsigned char, 8> encoded{};
        for (unsigned int shift = 0; shift < 64U; shift += 8U)
            encoded[shift / 8U] = static_cast<unsigned char>((value >> shift) & 0xffU);
        return append(encoded.data(), encoded.size());
    }

    bool string(const std::string& value)
    {
        if (value.size() > std::numeric_limits<std::uint32_t>::max() ||
            !u32(static_cast<std::uint32_t>(value.size())))
            return false;
        return append(reinterpret_cast<const unsigned char*>(value.data()), value.size());
    }

    std::vector<unsigned char> take()
    {
        return std::move(bytes_);
    }

    const std::vector<unsigned char>& bytes() const
    {
        return bytes_;
    }

  private:
    std::size_t limit_;
    std::vector<unsigned char> bytes_;
};

class Reader
{
  public:
    Reader(const unsigned char* bytes, std::size_t size, std::size_t string_limit)
        : bytes_(bytes), size_(size), string_limit_(string_limit)
    {
    }

    bool read(unsigned char* output, std::size_t size)
    {
        if (size > size_ || offset_ > size_ - size)
            return false;
        std::memcpy(output, bytes_ + offset_, size);
        offset_ += size;
        return true;
    }

    bool u8(std::uint8_t* value)
    {
        return read(value, 1U);
    }

    bool u32(std::uint32_t* value)
    {
        std::array<unsigned char, 4> encoded{};
        if (!read(encoded.data(), encoded.size()))
            return false;
        *value = 0;
        for (unsigned int shift = 0; shift < 32U; shift += 8U)
            *value |= static_cast<std::uint32_t>(encoded[shift / 8U]) << shift;
        return true;
    }

    bool u64(std::uint64_t* value)
    {
        std::array<unsigned char, 8> encoded{};
        if (!read(encoded.data(), encoded.size()))
            return false;
        *value = 0;
        for (unsigned int shift = 0; shift < 64U; shift += 8U)
            *value |= static_cast<std::uint64_t>(encoded[shift / 8U]) << shift;
        return true;
    }

    bool string(std::string* value)
    {
        std::uint32_t size = 0;
        if (!u32(&size) || size > string_limit_ || size > size_ || offset_ > size_ - size)
            return false;
        value->assign(reinterpret_cast<const char*>(bytes_ + offset_), size);
        offset_ += size;
        return true;
    }

    bool at_end() const
    {
        return offset_ == size_;
    }

  private:
    const unsigned char* bytes_;
    std::size_t size_;
    std::size_t string_limit_;
    std::size_t offset_ = 0;
};

bool encoded_string_size(std::size_t* size, const std::string& value)
{
    return value.size() <= std::numeric_limits<std::uint32_t>::max() && checked_add(size, 4U) &&
           checked_add(size, value.size());
}

bool encoded_transaction_size(const EditJournalTransactionRecord& transaction,
                              std::size_t* encoded_size)
{
    std::size_t size = 8U + 1U + 8U;
    if (transaction.kind == EditJournalTransactionRecord::Kind::logical_groups)
    {
        for (const EditJournalCommandRecord& command : transaction.commands)
        {
            if (!checked_add(&size, 1U) || !encoded_string_size(&size, command.authored_id) ||
                !checked_add(&size, 8U) || !encoded_string_size(&size, command.name) ||
                !checked_add(&size, 8U))
                return false;
            for (const EditJournalTargetRecord& member : command.members)
            {
                if (!checked_add(&size, 1U + 8U + 8U) ||
                    !encoded_string_size(&size, member.evidence_sha256))
                    return false;
            }
        }
    }
    else
    {
        for (const EditJournalProbeCommandRecord& command : transaction.probe_commands)
        {
            if (!checked_add(&size, 1U) || !encoded_string_size(&size, command.authored_id) ||
                !checked_add(&size, 8U + 1U + 8U) ||
                !encoded_string_size(&size, command.target_evidence_sha256) ||
                !encoded_string_size(&size, command.group_authored_id) ||
                !encoded_string_size(&size, command.key) ||
                !encoded_string_size(&size, command.value))
                return false;
        }
    }
    *encoded_size = size;
    return true;
}

bool journal_header_size(const SessionData& data, std::size_t* encoded_size)
{
    std::size_t size = kMagic.size() + 4U;
    const std::string inventory = edit_journal_target_inventory_sha256(data.snapshot);
    if (!encoded_string_size(&size, data.info.source_sha256) ||
        !encoded_string_size(&size, data.snapshot.brep_sha256) ||
        !encoded_string_size(&size, inventory) ||
        !encoded_string_size(&size, data.info.occt_version) || !checked_add(&size, 8U) ||
        !checked_add(&size, kDigestBytes))
        return false;
    *encoded_size = size;
    return true;
}

bool same_target(const EditJournalTargetRecord& left, const EditJournalTargetRecord& right)
{
    return left.kind == right.kind && left.target_index == right.target_index &&
           left.definition_index == right.definition_index &&
           left.evidence_sha256 == right.evidence_sha256;
}

bool same_transaction(const EditJournalTransactionRecord& left,
                      const EditJournalTransactionRecord& right)
{
    if (left.sequence != right.sequence || left.kind != right.kind ||
        left.commands.size() != right.commands.size() ||
        left.probe_commands.size() != right.probe_commands.size())
        return false;
    for (std::size_t command_index = 0; command_index < left.commands.size(); ++command_index)
    {
        const EditJournalCommandRecord& a = left.commands[command_index];
        const EditJournalCommandRecord& b = right.commands[command_index];
        if (a.kind != b.kind || a.authored_id != b.authored_id ||
            a.expected_revision != b.expected_revision || a.name != b.name ||
            a.members.size() != b.members.size())
            return false;
        for (std::size_t member_index = 0; member_index < a.members.size(); ++member_index)
        {
            if (!same_target(a.members[member_index], b.members[member_index]))
                return false;
        }
    }
    for (std::size_t index = 0; index < left.probe_commands.size(); ++index)
    {
        const EditJournalProbeCommandRecord& a = left.probe_commands[index];
        const EditJournalProbeCommandRecord& b = right.probe_commands[index];
        if (a.kind != b.kind || a.authored_id != b.authored_id ||
            a.expected_revision != b.expected_revision || a.target_kind != b.target_kind ||
            a.target_index != b.target_index ||
            a.target_evidence_sha256 != b.target_evidence_sha256 ||
            a.group_authored_id != b.group_authored_id || a.key != b.key || a.value != b.value)
            return false;
    }
    return true;
}

int replay_target(const SessionData* data, const EditJournalTargetRecord& stored,
                  std::string* handle, Status* status)
{
    if (stored.definition_index >= data->snapshot.definitions.size())
    {
        set_status(status, kConflict, "Edit-journal definition locator is out of range.");
        return kConflict;
    }
    const std::string& definition_handle =
        data->snapshot.definitions[stored.definition_index].handle;
    if (stored.kind == StepTopologyTargetKind::body)
    {
        if (stored.target_index >= data->snapshot.bodies.size())
        {
            set_status(status, kConflict, "Edit-journal body locator is out of range.");
            return kConflict;
        }
        const StepTopologyBody& body = data->snapshot.bodies[stored.target_index];
        if (body.definition_handle != definition_handle ||
            target_evidence(body, stored.definition_index) != stored.evidence_sha256)
        {
            set_status(status, kConflict, "Edit-journal body evidence does not match the source.");
            return kConflict;
        }
        *handle = body.handle;
        return 0;
    }
    if (stored.kind == StepTopologyTargetKind::face)
    {
        if (stored.target_index >= data->snapshot.faces.size())
        {
            set_status(status, kConflict, "Edit-journal face locator is out of range.");
            return kConflict;
        }
        const StepTopologyFace& face = data->snapshot.faces[stored.target_index];
        if (face.definition_handle != definition_handle ||
            target_evidence(face, stored.definition_index) != stored.evidence_sha256)
        {
            set_status(status, kConflict, "Edit-journal face evidence does not match the source.");
            return kConflict;
        }
        *handle = face.handle;
        return 0;
    }
    set_status(status, kInvalidArgument, "Edit-journal target kind is not replayable.");
    return kInvalidArgument;
}

} // namespace

void set_edit_journal_replay_apply_entry_hook_for_test(EditJournalReplayApplyEntryHook hook,
                                                       void* context)
{
    replay_apply_entry_hook = hook;
    replay_apply_entry_context = context;
}

std::string edit_journal_target_inventory_sha256(const StepTopologySnapshot& snapshot)
{
    Sha256Builder hash;
    hash_string(&hash, "geometer.step_topology_target_inventory.a0");
    hash_u64(&hash, snapshot.free_shape_count);
    hash_u64(&hash, snapshot.component_label_count);

    std::unordered_map<std::string, std::size_t> definitions;
    std::unordered_map<std::string, std::size_t> root_occurrences;
    std::unordered_map<std::string, std::size_t> occurrences;
    std::unordered_map<std::string, std::size_t> bodies;
    std::unordered_map<std::string, std::size_t> shells;
    std::unordered_map<std::string, std::size_t> faces;
    const auto index = [](const auto& records, auto* output)
    {
        output->reserve(records.size());
        for (std::size_t ordinal = 0; ordinal < records.size(); ++ordinal)
            output->emplace(records[ordinal].handle, ordinal);
    };
    index(snapshot.definitions, &definitions);
    index(snapshot.root_occurrences, &root_occurrences);
    index(snapshot.occurrences, &occurrences);
    index(snapshot.bodies, &bodies);
    index(snapshot.shells, &shells);
    index(snapshot.faces, &faces);
    const auto hash_reference = [&hash](const auto& indexes, const std::string& handle)
    {
        const auto found = indexes.find(handle);
        hash_u64(&hash, found == indexes.end() ? std::numeric_limits<std::uint64_t>::max()
                                               : static_cast<std::uint64_t>(found->second));
    };
    const auto hash_references = [&hash_reference, &hash](const auto& indexes, const auto& handles)
    {
        hash_u64(&hash, handles.size());
        for (const std::string& handle : handles)
            hash_reference(indexes, handle);
    };

    hash_u64(&hash, snapshot.definitions.size());
    for (const StepTopologyDefinition& definition : snapshot.definitions)
    {
        hash_u64(&hash, definition.is_assembly ? 1U : 0U);
        hash_label(&hash, definition.label);
        hash_references(bodies, definition.body_handles);
    }
    hash_u64(&hash, snapshot.root_occurrences.size());
    for (const StepTopologyRootOccurrence& occurrence : snapshot.root_occurrences)
    {
        hash_reference(definitions, occurrence.definition_handle);
        hash_doubles(&hash, occurrence.transform);
        hash_label(&hash, occurrence.label);
    }
    hash_u64(&hash, snapshot.occurrences.size());
    for (const StepTopologyOccurrence& occurrence : snapshot.occurrences)
    {
        hash_reference(definitions, occurrence.definition_handle);
        const auto root_parent = root_occurrences.find(occurrence.parent_occurrence_handle);
        const auto occurrence_parent = occurrences.find(occurrence.parent_occurrence_handle);
        if (occurrence.parent_occurrence_handle.empty())
        {
            hash_u64(&hash, 0U);
            hash_u64(&hash, 0U);
        }
        else if (root_parent != root_occurrences.end())
        {
            hash_u64(&hash, 1U);
            hash_u64(&hash, root_parent->second);
        }
        else if (occurrence_parent != occurrences.end())
        {
            hash_u64(&hash, 2U);
            hash_u64(&hash, occurrence_parent->second);
        }
        else
        {
            hash_u64(&hash, 3U);
            hash_u64(&hash, 0U);
        }
        hash_u64(&hash, occurrence.depth);
        hash_doubles(&hash, occurrence.transform);
        hash_label(&hash, occurrence.label);
    }
    hash_u64(&hash, snapshot.bodies.size());
    for (const StepTopologyBody& body : snapshot.bodies)
    {
        hash_reference(definitions, body.definition_handle);
        hash_string(&hash, body.topology_kind);
        hash_references(shells, body.shell_handles);
        hash_references(faces, body.face_handles);
        hash_doubles(&hash, body.bounds);
        hash_double(&hash, body.volume);
        hash_label(&hash, body.label);
    }
    hash_u64(&hash, snapshot.shells.size());
    for (const StepTopologyShell& shell : snapshot.shells)
    {
        hash_reference(definitions, shell.definition_handle);
        hash_references(bodies, shell.body_handles);
        hash_references(faces, shell.face_handles);
        hash_label(&hash, shell.label);
    }
    hash_u64(&hash, snapshot.faces.size());
    for (const StepTopologyFace& face : snapshot.faces)
    {
        hash_reference(definitions, face.definition_handle);
        hash_references(bodies, face.body_handles);
        hash_references(shells, face.shell_handles);
        hash_doubles(&hash, face.bounds);
        hash_double(&hash, face.area);
        hash_doubles(&hash, face.centroid);
        hash_label(&hash, face.label);
    }
    return hash.hex_digest();
}

int initialize_edit_journal_accounting(SessionData* data, Status* status)
{
    std::size_t encoded_size = 0;
    if (!journal_header_size(*data, &encoded_size) ||
        encoded_size > data->limits.max_edit_journal_bytes)
    {
        set_status(status, kResourceLimit,
                   "The empty edit-journal checkpoint exceeds its byte limit.");
        return kResourceLimit;
    }
    data->edit_journal_encoded_bytes = encoded_size;
    return 0;
}

int validate_edit_journal_append(const SessionData* data, const EditJournalTransactionRecord& entry,
                                 std::size_t* projected_bytes, Status* status)
{
    std::size_t entry_bytes = 0;
    if (projected_bytes == nullptr || data->edit_journal_encoded_bytes == 0 ||
        !encoded_transaction_size(entry, &entry_bytes) ||
        data->edit_journal_encoded_bytes > data->limits.max_edit_journal_bytes ||
        entry_bytes > data->limits.max_edit_journal_bytes - data->edit_journal_encoded_bytes)
    {
        set_status(status, kResourceLimit,
                   "Edit-journal transaction exceeds the checkpoint byte limit.");
        return kResourceLimit;
    }
    *projected_bytes = data->edit_journal_encoded_bytes + entry_bytes;
    return 0;
}

int account_edit_journal_strings(SessionData* data, const StepTopologyCancellation* cancellation,
                                 Status* status)
{
    const std::size_t before = data->total_string_bytes;
    for (const EditJournalTransactionRecord& transaction : data->edit_journal)
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            set_status(status, kCancelled, "Edit-journal accounting was cancelled.");
            return kCancelled;
        }
        for (const EditJournalCommandRecord& command : transaction.commands)
        {
            if (cancellation != nullptr && cancellation->is_cancelled())
            {
                set_status(status, kCancelled, "Edit-journal accounting was cancelled.");
                return kCancelled;
            }
            if (!account_string(data, command.authored_id, status) ||
                !account_string(data, command.name, status))
                return kResourceLimit;
            for (const EditJournalTargetRecord& member : command.members)
            {
                if (cancellation != nullptr && cancellation->is_cancelled())
                {
                    set_status(status, kCancelled, "Edit-journal accounting was cancelled.");
                    return kCancelled;
                }
                if (!account_string(data, member.evidence_sha256, status))
                    return kResourceLimit;
            }
        }
        for (const EditJournalProbeCommandRecord& command : transaction.probe_commands)
        {
            if (cancellation != nullptr && cancellation->is_cancelled())
            {
                set_status(status, kCancelled, "Edit-journal accounting was cancelled.");
                return kCancelled;
            }
            if (!account_string(data, command.authored_id, status) ||
                !account_string(data, command.target_evidence_sha256, status) ||
                !account_string(data, command.group_authored_id, status) ||
                !account_string(data, command.key, status) ||
                !account_string(data, command.value, status))
                return kResourceLimit;
        }
    }
    data->journal_string_bytes = data->total_string_bytes - before;
    return 0;
}

int stage_edit_journal_transaction(const SessionData* data,
                                   const StepTopologyGroupTransaction& transaction,
                                   const StepTopologyCancellation* cancellation,
                                   EditJournalTransactionRecord* entry,
                                   std::size_t* entry_string_bytes, Status* status)
{
    if (entry == nullptr || entry_string_bytes == nullptr)
    {
        set_status(status, kInvalidArgument, "Edit-journal staging output is null.");
        return kInvalidArgument;
    }
    *entry = {};
    *entry_string_bytes = 0;
    const auto cancelled = [&]()
    {
        if (cancellation == nullptr || !cancellation->is_cancelled())
            return false;
        set_status(status, kCancelled, "Edit-journal transaction staging was cancelled.");
        return true;
    };
    if (cancelled())
        return kCancelled;
    if (data->edit_journal.size() >= data->limits.max_edit_journal_transactions ||
        data->edit_journal.size() == std::numeric_limits<std::uint64_t>::max())
    {
        set_status(status, kResourceLimit, "Edit-journal transaction limit is exhausted.");
        return kResourceLimit;
    }

    std::unordered_map<std::string, std::size_t> definitions;
    definitions.reserve(data->snapshot.definitions.size());
    for (std::size_t index = 0; index < data->snapshot.definitions.size(); ++index)
    {
        if (cancelled())
            return kCancelled;
        definitions.emplace(data->snapshot.definitions[index].handle, index);
    }

    struct TargetIndex
    {
        StepTopologyTargetKind kind = StepTopologyTargetKind::face;
        std::size_t index = 0;
        std::size_t definition_index = 0;
        std::string evidence_sha256;
    };
    std::unordered_map<std::string, TargetIndex> targets;
    targets.reserve(data->snapshot.bodies.size() + data->snapshot.faces.size());
    for (std::size_t index = 0; index < data->snapshot.bodies.size(); ++index)
    {
        if (cancelled())
            return kCancelled;
        const StepTopologyBody& body = data->snapshot.bodies[index];
        const auto definition = definitions.find(body.definition_handle);
        if (definition == definitions.end())
        {
            set_status(status, kInternalFailure,
                       "Body definition is absent while staging journal.");
            return kInternalFailure;
        }
        targets.emplace(body.handle,
                        TargetIndex{StepTopologyTargetKind::body, index, definition->second,
                                    target_evidence(body, definition->second)});
    }
    for (std::size_t index = 0; index < data->snapshot.faces.size(); ++index)
    {
        if (cancelled())
            return kCancelled;
        const StepTopologyFace& face = data->snapshot.faces[index];
        const auto definition = definitions.find(face.definition_handle);
        if (definition == definitions.end())
        {
            set_status(status, kInternalFailure,
                       "Face definition is absent while staging journal.");
            return kInternalFailure;
        }
        targets.emplace(face.handle,
                        TargetIndex{StepTopologyTargetKind::face, index, definition->second,
                                    target_evidence(face, definition->second)});
    }

    entry->sequence = static_cast<std::uint64_t>(data->edit_journal.size()) + 1U;
    entry->kind = EditJournalTransactionRecord::Kind::logical_groups;
    entry->commands.reserve(transaction.commands.size());
    for (const StepTopologyGroupCommand& command : transaction.commands)
    {
        if (cancelled())
            return kCancelled;
        EditJournalCommandRecord recorded;
        recorded.kind = command.kind;
        recorded.authored_id = command.authored_id;
        recorded.expected_revision = command.expected_revision;
        recorded.name = command.name;
        if (!checked_add(entry_string_bytes, recorded.authored_id.size()) ||
            !checked_add(entry_string_bytes, recorded.name.size()))
        {
            set_status(status, kResourceLimit, "Edit-journal string accounting overflowed.");
            return kResourceLimit;
        }
        recorded.members.reserve(command.member_handles.size());
        for (const std::string& handle : command.member_handles)
        {
            if (cancelled())
                return kCancelled;
            const auto found = targets.find(handle);
            if (found == targets.end())
            {
                set_status(status, kUnknownTarget,
                           "Edit-journal member handle is stale, unknown, or unsupported.");
                return kUnknownTarget;
            }
            recorded.members.push_back({found->second.kind, found->second.index,
                                        found->second.definition_index,
                                        found->second.evidence_sha256});
            if (!checked_add(entry_string_bytes, found->second.evidence_sha256.size()))
            {
                set_status(status, kResourceLimit, "Edit-journal string accounting overflowed.");
                return kResourceLimit;
            }
        }
        entry->commands.push_back(std::move(recorded));
    }
    std::size_t projected_bytes = 0;
    return validate_edit_journal_append(data, *entry, &projected_bytes, status);
}

int encode_edit_journal(const SessionData* data, StepTopologyEditJournalCheckpoint* checkpoint,
                        Status* status)
{
    if (checkpoint == nullptr)
    {
        set_status(status, kInvalidArgument, "Edit-journal checkpoint output is null.");
        return kInvalidArgument;
    }
    *checkpoint = {};
    try
    {
        Writer writer(data->limits.max_edit_journal_bytes);
        const std::string target_inventory_sha256 =
            edit_journal_target_inventory_sha256(data->snapshot);
        if (!writer.append(kMagic.data(), kMagic.size()) || !writer.u32(kVersion) ||
            !writer.string(data->info.source_sha256) ||
            !writer.string(data->snapshot.brep_sha256) || !writer.string(target_inventory_sha256) ||
            !writer.string(data->info.occt_version) ||
            !writer.u64(static_cast<std::uint64_t>(data->edit_journal.size())))
        {
            set_status(status, kResourceLimit, "Edit-journal checkpoint exceeds its byte limit.");
            return kResourceLimit;
        }
        for (const EditJournalTransactionRecord& transaction : data->edit_journal)
        {
            const std::size_t command_count =
                transaction.kind == EditJournalTransactionRecord::Kind::logical_groups
                    ? transaction.commands.size()
                    : transaction.probe_commands.size();
            if (!writer.u64(transaction.sequence) ||
                !writer.u8(static_cast<std::uint8_t>(transaction.kind)) ||
                !writer.u64(static_cast<std::uint64_t>(command_count)))
            {
                set_status(status, kResourceLimit,
                           "Edit-journal checkpoint exceeds its byte limit.");
                return kResourceLimit;
            }
            if (transaction.kind == EditJournalTransactionRecord::Kind::logical_groups)
            {
                for (const EditJournalCommandRecord& command : transaction.commands)
                {
                    if (!writer.u8(static_cast<std::uint8_t>(command.kind)) ||
                        !writer.string(command.authored_id) ||
                        !writer.u64(command.expected_revision) || !writer.string(command.name) ||
                        !writer.u64(static_cast<std::uint64_t>(command.members.size())))
                    {
                        set_status(status, kResourceLimit,
                                   "Edit-journal checkpoint exceeds its byte limit.");
                        return kResourceLimit;
                    }
                    for (const EditJournalTargetRecord& member : command.members)
                    {
                        if (!writer.u8(static_cast<std::uint8_t>(member.kind)) ||
                            !writer.u64(static_cast<std::uint64_t>(member.target_index)) ||
                            !writer.u64(static_cast<std::uint64_t>(member.definition_index)) ||
                            !writer.string(member.evidence_sha256))
                        {
                            set_status(status, kResourceLimit,
                                       "Edit-journal checkpoint exceeds its byte limit.");
                            return kResourceLimit;
                        }
                    }
                }
            }
            else
            {
                for (const EditJournalProbeCommandRecord& command : transaction.probe_commands)
                {
                    if (!writer.u8(static_cast<std::uint8_t>(command.kind)) ||
                        !writer.string(command.authored_id) ||
                        !writer.u64(command.expected_revision) ||
                        !writer.u8(static_cast<std::uint8_t>(command.target_kind)) ||
                        !writer.u64(static_cast<std::uint64_t>(command.target_index)) ||
                        !writer.string(command.target_evidence_sha256) ||
                        !writer.string(command.group_authored_id) || !writer.string(command.key) ||
                        !writer.string(command.value))
                    {
                        set_status(status, kResourceLimit,
                                   "Edit-journal checkpoint exceeds its byte limit.");
                        return kResourceLimit;
                    }
                }
            }
        }
        const auto digest = sha256(writer.bytes().data(), writer.bytes().size());
        if (!writer.append(digest.data(), digest.size()))
        {
            set_status(status, kResourceLimit, "Edit-journal checkpoint exceeds its byte limit.");
            return kResourceLimit;
        }
        if (writer.bytes().size() != data->edit_journal_encoded_bytes)
        {
            set_status(status, kInternalFailure,
                       "Edit-journal byte accounting does not match its encoding.");
            return kInternalFailure;
        }
        checkpoint->source_sha256 = data->info.source_sha256;
        checkpoint->source_brep_sha256 = data->snapshot.brep_sha256;
        checkpoint->target_inventory_sha256 = target_inventory_sha256;
        checkpoint->occt_version = data->info.occt_version;
        checkpoint->transaction_count = data->edit_journal.size();
        checkpoint->bytes = writer.take();
        checkpoint->content_sha256 = sha256_hex(checkpoint->bytes.data(), checkpoint->bytes.size());
        set_status(status, 0, "");
        return 0;
    }
    catch (const std::exception& error)
    {
        *checkpoint = {};
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

int decode_edit_journal(const unsigned char* bytes, std::size_t size,
                        const StepTopologyLimits& limits, std::string* source_sha256,
                        std::string* source_brep_sha256, std::string* target_inventory_sha256,
                        std::string* occt_version, const StepTopologyCancellation* cancellation,
                        std::vector<EditJournalTransactionRecord>* transactions, Status* status)
{
    if (bytes == nullptr || source_sha256 == nullptr || source_brep_sha256 == nullptr ||
        target_inventory_sha256 == nullptr || occt_version == nullptr || transactions == nullptr ||
        size < kMagic.size() + 4U + kDigestBytes || size > limits.max_edit_journal_bytes)
    {
        set_status(status, kInvalidArgument, "Edit-journal checkpoint input is invalid.");
        return kInvalidArgument;
    }
    *source_sha256 = {};
    *source_brep_sha256 = {};
    *target_inventory_sha256 = {};
    *occt_version = {};
    transactions->clear();
    try
    {
        const std::size_t payload_size = size - kDigestBytes;
        const auto digest = sha256(bytes, payload_size);
        if (!std::equal(digest.begin(), digest.end(), bytes + payload_size))
        {
            set_status(status, kConflict, "Edit-journal checkpoint digest is invalid.");
            return kConflict;
        }
        Reader reader(bytes, payload_size, limits.max_string_bytes);
        std::array<unsigned char, 8> magic{};
        std::uint32_t version = 0;
        std::uint64_t transaction_count = 0;
        if (!reader.read(magic.data(), magic.size()) || magic != kMagic || !reader.u32(&version) ||
            version != kVersion || !reader.string(source_sha256) ||
            !reader.string(source_brep_sha256) || !reader.string(target_inventory_sha256) ||
            !reader.string(occt_version) || !reader.u64(&transaction_count) ||
            transaction_count > limits.max_edit_journal_transactions)
        {
            set_status(status, kInvalidArgument,
                       "Edit-journal checkpoint header is malformed or unsupported.");
            return kInvalidArgument;
        }
        if (!valid_sha256(*source_sha256) || !valid_sha256(*source_brep_sha256) ||
            !valid_sha256(*target_inventory_sha256) || occt_version->empty())
        {
            set_status(status, kInvalidArgument, "Edit-journal source evidence is malformed.");
            return kInvalidArgument;
        }
        transactions->reserve(static_cast<std::size_t>(transaction_count));
        for (std::uint64_t transaction_index = 0; transaction_index < transaction_count;
             ++transaction_index)
        {
            if (cancellation != nullptr && cancellation->is_cancelled())
            {
                set_status(status, kCancelled, "Edit-journal decoding was cancelled.");
                return kCancelled;
            }
            EditJournalTransactionRecord transaction;
            std::uint8_t transaction_kind = 0;
            std::uint64_t command_count = 0;
            if (!reader.u64(&transaction.sequence) ||
                transaction.sequence != transaction_index + 1U || !reader.u8(&transaction_kind) ||
                transaction_kind > static_cast<std::uint8_t>(
                                       EditJournalTransactionRecord::Kind::metadata_probes) ||
                !reader.u64(&command_count) || command_count == 0 ||
                command_count > std::max(limits.max_logical_groups, limits.max_metadata_probes))
            {
                set_status(status, kInvalidArgument,
                           "Edit-journal transaction sequence or command count is invalid.");
                return kInvalidArgument;
            }
            transaction.kind = static_cast<EditJournalTransactionRecord::Kind>(transaction_kind);
            if (transaction.kind == EditJournalTransactionRecord::Kind::logical_groups)
            {
                if (command_count > limits.max_logical_groups)
                {
                    set_status(status, kInvalidArgument,
                               "Edit-journal logical-group command count is invalid.");
                    return kInvalidArgument;
                }
                transaction.commands.reserve(static_cast<std::size_t>(command_count));
                for (std::uint64_t command_index = 0; command_index < command_count;
                     ++command_index)
                {
                    if (cancellation != nullptr && cancellation->is_cancelled())
                    {
                        set_status(status, kCancelled, "Edit-journal decoding was cancelled.");
                        return kCancelled;
                    }
                    EditJournalCommandRecord command;
                    std::uint8_t kind = 0;
                    std::uint64_t member_count = 0;
                    if (!reader.u8(&kind) ||
                        kind > static_cast<std::uint8_t>(StepTopologyGroupCommandKind::erase) ||
                        !reader.string(&command.authored_id) ||
                        !reader.u64(&command.expected_revision) || !reader.string(&command.name) ||
                        !reader.u64(&member_count) || member_count > limits.max_group_members)
                    {
                        set_status(status, kInvalidArgument, "Edit-journal command is malformed.");
                        return kInvalidArgument;
                    }
                    command.kind = static_cast<StepTopologyGroupCommandKind>(kind);
                    command.members.reserve(static_cast<std::size_t>(member_count));
                    for (std::uint64_t member_index = 0; member_index < member_count;
                         ++member_index)
                    {
                        if (cancellation != nullptr && cancellation->is_cancelled())
                        {
                            set_status(status, kCancelled, "Edit-journal decoding was cancelled.");
                            return kCancelled;
                        }
                        EditJournalTargetRecord member;
                        std::uint8_t target_kind = 0;
                        std::uint64_t target_index = 0;
                        std::uint64_t definition_index = 0;
                        if (!reader.u8(&target_kind) ||
                            (target_kind !=
                                 static_cast<std::uint8_t>(StepTopologyTargetKind::body) &&
                             target_kind !=
                                 static_cast<std::uint8_t>(StepTopologyTargetKind::face)) ||
                            !reader.u64(&target_index) || !reader.u64(&definition_index) ||
                            target_index > std::numeric_limits<std::size_t>::max() ||
                            definition_index > std::numeric_limits<std::size_t>::max() ||
                            !reader.string(&member.evidence_sha256) ||
                            !valid_sha256(member.evidence_sha256))
                        {
                            set_status(status, kInvalidArgument,
                                       "Edit-journal member locator is malformed.");
                            return kInvalidArgument;
                        }
                        member.kind = static_cast<StepTopologyTargetKind>(target_kind);
                        member.target_index = static_cast<std::size_t>(target_index);
                        member.definition_index = static_cast<std::size_t>(definition_index);
                        command.members.push_back(std::move(member));
                    }
                    transaction.commands.push_back(std::move(command));
                }
            }
            else
            {
                if (command_count > limits.max_metadata_probes)
                {
                    set_status(status, kInvalidArgument,
                               "Edit-journal metadata-probe command count is invalid.");
                    return kInvalidArgument;
                }
                transaction.probe_commands.reserve(static_cast<std::size_t>(command_count));
                for (std::uint64_t command_index = 0; command_index < command_count;
                     ++command_index)
                {
                    if (cancellation != nullptr && cancellation->is_cancelled())
                    {
                        set_status(status, kCancelled, "Edit-journal decoding was cancelled.");
                        return kCancelled;
                    }
                    EditJournalProbeCommandRecord command;
                    std::uint8_t kind = 0;
                    std::uint8_t target_kind = 0;
                    std::uint64_t target_index = 0;
                    if (!reader.u8(&kind) ||
                        kind > static_cast<std::uint8_t>(StepTopologyProbeCommandKind::erase) ||
                        !reader.string(&command.authored_id) ||
                        !reader.u64(&command.expected_revision) || !reader.u8(&target_kind) ||
                        target_kind >
                            static_cast<std::uint8_t>(StepTopologyProbeTargetKind::logical_group) ||
                        !reader.u64(&target_index) ||
                        target_index > std::numeric_limits<std::size_t>::max() ||
                        !reader.string(&command.target_evidence_sha256) ||
                        (!command.target_evidence_sha256.empty() &&
                         !valid_sha256(command.target_evidence_sha256)) ||
                        !reader.string(&command.group_authored_id) ||
                        !reader.string(&command.key) || !reader.string(&command.value))
                    {
                        set_status(status, kInvalidArgument,
                                   "Edit-journal metadata-probe command is malformed.");
                        return kInvalidArgument;
                    }
                    command.kind = static_cast<StepTopologyProbeCommandKind>(kind);
                    command.target_kind = static_cast<StepTopologyProbeTargetKind>(target_kind);
                    command.target_index = static_cast<std::size_t>(target_index);
                    transaction.probe_commands.push_back(std::move(command));
                }
            }
            transactions->push_back(std::move(transaction));
        }
        if (!reader.at_end())
        {
            set_status(status, kInvalidArgument, "Edit-journal checkpoint has trailing payload.");
            return kInvalidArgument;
        }
        set_status(status, 0, "");
        return 0;
    }
    catch (const std::exception& error)
    {
        transactions->clear();
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

int replay_edit_journal(SessionData* data,
                        const std::vector<EditJournalTransactionRecord>& transactions,
                        const StepTopologyCancellation* cancellation,
                        StepTopologyEditJournalRestoreResult* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, kInvalidArgument, "Edit-journal replay output is null.");
        return kInvalidArgument;
    }
    *result = {};
    const auto invoke_apply_entry_hook = []()
    {
        if (replay_apply_entry_hook == nullptr)
            return;
        const EditJournalReplayApplyEntryHook hook = replay_apply_entry_hook;
        void* context = replay_apply_entry_context;
        replay_apply_entry_hook = nullptr;
        replay_apply_entry_context = nullptr;
        hook(context);
    };
    std::size_t topology_items = data->snapshot.definitions.size();
    for (std::size_t count :
         {data->snapshot.root_occurrences.size(), data->snapshot.occurrences.size(),
          data->snapshot.bodies.size(), data->snapshot.shells.size(), data->snapshot.faces.size()})
    {
        if (!checked_add(&topology_items, count))
        {
            set_status(status, kResourceLimit, "Edit-journal replay work accounting overflowed.");
            return kResourceLimit;
        }
    }
    if (!checked_add(&topology_items, data->snapshot.brep_digest_work_items) ||
        !checked_add(&topology_items, data->snapshot.source_transfer_work_items))
    {
        set_status(status, kResourceLimit, "Edit-journal replay work accounting overflowed.");
        return kResourceLimit;
    }
    std::size_t replay_work = 0;
    std::size_t cumulative_commands = 0;
    std::size_t cumulative_members = 0;
    std::size_t authored_state_upper_bound = 0;
    for (std::size_t index = 0; index < transactions.size(); ++index)
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            set_status(status, kCancelled, "Edit-journal replay metering was cancelled.");
            return kCancelled;
        }
        const EditJournalTransactionRecord& transaction = transactions[index];
        std::size_t command_count = 0;
        std::size_t member_count = 0;
        if (transaction.kind == EditJournalTransactionRecord::Kind::logical_groups)
        {
            command_count = transaction.commands.size();
            for (const EditJournalCommandRecord& command : transaction.commands)
            {
                if (cancellation != nullptr && cancellation->is_cancelled())
                {
                    set_status(status, kCancelled, "Edit-journal replay metering was cancelled.");
                    return kCancelled;
                }
                if (!checked_add(&member_count, command.members.size()))
                {
                    set_status(status, kResourceLimit,
                               "Edit-journal replay work accounting overflowed.");
                    return kResourceLimit;
                }
            }
        }
        else
        {
            command_count = transaction.probe_commands.size();
        }
        if (!checked_add(&cumulative_commands, command_count) ||
            !checked_add(&cumulative_members, member_count) ||
            !checked_add(&authored_state_upper_bound, command_count))
        {
            set_status(status, kResourceLimit, "Edit-journal replay work accounting overflowed.");
            return kResourceLimit;
        }
        // Each apply builds topology locator indexes and a new snapshot, scans/copies the
        // accumulated authored state, and recounts the complete journal history. Monotonic
        // command/member upper bounds deliberately overcharge erased or replaced records.
        std::size_t transaction_work = index + 1U;
        if (!checked_add_scaled(&transaction_work, topology_items, 4U) ||
            !checked_add_scaled(&transaction_work, cumulative_commands, 4U) ||
            !checked_add_scaled(&transaction_work, cumulative_members, 3U) ||
            !checked_add_scaled(&transaction_work, authored_state_upper_bound, 3U))
        {
            set_status(status, kResourceLimit, "Edit-journal replay work accounting overflowed.");
            return kResourceLimit;
        }
        if (!checked_add(&replay_work, transaction_work) ||
            replay_work > data->limits.max_edit_journal_replay_work_items)
        {
            set_status(status, kResourceLimit,
                       "Edit-journal replay exceeds its configured work limit.");
            return kResourceLimit;
        }
    }
    for (const EditJournalTransactionRecord& stored : transactions)
    {
        if (cancellation != nullptr && cancellation->is_cancelled())
        {
            set_status(status, kCancelled, "Edit-journal replay was cancelled.");
            return kCancelled;
        }
        if (stored.kind == EditJournalTransactionRecord::Kind::logical_groups)
        {
            StepTopologyGroupTransaction transaction;
            transaction.expected_generation = data->info.generation;
            transaction.commands.reserve(stored.commands.size());
            for (const EditJournalCommandRecord& stored_command : stored.commands)
            {
                if (cancellation != nullptr && cancellation->is_cancelled())
                {
                    set_status(status, kCancelled,
                               "Edit-journal command reconstruction was cancelled.");
                    return kCancelled;
                }
                StepTopologyGroupCommand command;
                command.kind = stored_command.kind;
                command.authored_id = stored_command.authored_id;
                command.expected_revision = stored_command.expected_revision;
                command.name = stored_command.name;
                command.member_handles.reserve(stored_command.members.size());
                for (const EditJournalTargetRecord& member : stored_command.members)
                {
                    if (cancellation != nullptr && cancellation->is_cancelled())
                    {
                        set_status(status, kCancelled,
                                   "Edit-journal member reconstruction was cancelled.");
                        return kCancelled;
                    }
                    std::string handle;
                    const int target_code = replay_target(data, member, &handle, status);
                    if (target_code != 0)
                        return target_code;
                    command.member_handles.push_back(std::move(handle));
                }
                transaction.commands.push_back(std::move(command));
            }
            StepTopologyGroupTransactionResult group_result;
            invoke_apply_entry_hook();
            const int code = apply_logical_group_transaction(
                data, transaction, cancellation, nullptr, nullptr, &group_result, status);
            if (code != 0)
                return code;
        }
        else
        {
            StepTopologyProbeTransaction transaction;
            const int restore_code =
                restore_probe_transaction(data, stored, cancellation, &transaction, status);
            if (restore_code != 0)
                return restore_code;
            StepTopologyProbeTransactionResult probe_result;
            invoke_apply_entry_hook();
            const int code = apply_metadata_probe_transaction(
                data, transaction, cancellation, nullptr, nullptr, &probe_result, status);
            if (code != 0)
                return code;
        }
        if (data->edit_journal.empty() || !same_transaction(data->edit_journal.back(), stored))
        {
            set_status(status, kConflict,
                       "Replayed edit-journal transaction did not reproduce canonical evidence.");
            return kConflict;
        }
    }
    result->session = data->info;
    const int group_code =
        publish_logical_groups(data, data->logical_groups, cancellation, &result->groups, status);
    if (group_code != 0)
        return group_code;
    const int probe_code =
        publish_metadata_probes(data, data->metadata_probes, cancellation, &result->probes, status);
    if (probe_code != 0)
        return probe_code;
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer::step_topology_internal
