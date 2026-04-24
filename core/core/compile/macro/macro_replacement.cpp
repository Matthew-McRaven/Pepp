#include "core/compile/macro/macro_replacement.hpp"
#include <regex>

std::string pepp::tc::replace_macro_arguments(std::string_view input, MacroReplacements rep) {
  // Match \() or \+ or \@. Also match any backslash followed by something looking like an identifier
  static const auto pattern = R"(\\\(\s*\)|\\\+|\\@|\\[a-zA-Z_]\w*)";
  static const std::regex re(pattern);

  // The result will be built incrementally. Reserve
  std::string result;
  result.reserve(input.size());

  // Use const char* iterators to avoid unecessary std::string allocations.
  const char *data = input.data();
  //
  const std::size_t len = input.size();
  // Keep track of the last byte position which we copied to
  std::size_t last_pos = 0;
  // Construct a match itrator over our entire input string.
  std::cregex_iterator it(data, data + len, re);

  // End sentintel is default-constructed.
  for (; it != std::cregex_iterator{}; ++it) {
    // Extract the position and length of this match.
    auto pos = static_cast<std::size_t>(it->position()), mlen = static_cast<std::size_t>(it->length());
    // Copy all text between the last match and this match into the result.
    result.append(data + last_pos, pos - last_pos);

    // Check if the matched text is one of our replacements.
    std::string_view key(data + pos, mlen);
    auto found = rep.find(std::string(key));

    // Insert the replacement text on a match, otherwise preserve the original value.
    result.append(found != rep.end() ? std::string_view(found->second) : key);
    last_pos = pos + mlen;
  }

  // Copy any remaining text after the last match.
  result.append(data + last_pos, len - last_pos);
  return result;
}

pepp::tc::MacroReplacements pepp::tc::MacroCounters::counters_for(std::string name) {
  MacroReplacements ret;
  ret["\\()"] = "";
  ret["\\@"] = std::to_string(_total++);
  ret["\\+"] = std::to_string(_counters[name]++);
  return ret;
}
