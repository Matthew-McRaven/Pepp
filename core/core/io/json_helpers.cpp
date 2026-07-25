#include "json_helpers.hpp"
#include <nlohmann/json.hpp>

std::string templatize(std::string input, const std::map<std::string, std::string> &substitutions) {
  std::string result;
  result.reserve(input.size());
  // Try to match each position against a substitution.
  for (std::size_t pos = 0; pos < input.size();) {
    bool matched = false;
    for (const auto &[search, replace] : substitutions) {
      // If we have a hit, copy replace into result and advance position.
      if (!search.empty() && input.compare(pos, search.size(), search) == 0) {
        result += replace;
        pos += search.size();
        matched = true;
        break;
      }
    }
    // If position does not match, copy that character forward.
    if (!matched) result += input[pos++];
  }
  return result;
}

void templatize(nlohmann::json &object, const std::map<std::string, std::string> &substitutions) {
  for (auto &[key, value] : object.items()) {
    if (value.is_object()) templatize(value, substitutions);
    else if (value.is_string()) object[key] = templatize(value.get<std::string>(), substitutions);
    else if (value.is_array()) {
      for (auto &element : value) templatize(element, substitutions);
    }
  }
}
