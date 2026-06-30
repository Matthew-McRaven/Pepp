#pragma once

#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>

std::string templatize(std::string, const std::map<std::string, std::string> &substitutions);
// Perform template substitution on all string values in a JSON object, recursively.
void templatize(nlohmann::json &object, const std::map<std::string, std::string> &substitutions);