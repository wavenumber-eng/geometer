#pragma once

#include <string_view>

namespace geometer
{
namespace step_media_type_detail
{
// Internal STEP input admission only. Outputs and unrelated catalog media stay exact.
bool accepts(std::string_view value);
} // namespace step_media_type_detail
} // namespace geometer
