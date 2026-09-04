#pragma once

#include <string>

namespace geometer::cli
{

/** Execute the governed indexed-mesh HLR operation and write its A0 result JSON. */
int project_indexed_mesh_file(const std::string& input_path, const std::string& output_path,
                              const std::string& base_options_json,
                              const std::string& override_options_json, std::string* error_message);

} // namespace geometer::cli
