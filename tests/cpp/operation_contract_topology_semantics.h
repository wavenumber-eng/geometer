#pragma once

#include <rapidjson/document.h>

#include <string>
#include <vector>

bool validate_step_topology_semantics(const rapidjson::Value& vector,
                                      const std::vector<unsigned char>& data,
                                      const std::string& vector_root);
