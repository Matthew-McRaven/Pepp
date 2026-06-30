#include "json_helpers.hpp"
#include <nlohmann/json.hpp>

std::string templatize(std::string input, const std::map<std::string, std::string> &substitutions) {
  for (const auto &[search, replace] : substitutions) {
    for (std::size_t pos = input.find(search); pos != std::string::npos; pos = input.find(search, pos + input.size()))
      input.replace(pos, search.size(), replace);
  }
  return input;
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
