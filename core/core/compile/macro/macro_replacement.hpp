#pragma once
#include <string>
#include <unordered_map>
#include "core/ds/string_compare.hpp"

namespace pepp::tc {

// Used to perform textual substituion of macro arguments into macro body, minimizing intermediate allocations.
// Replace all occurences of the keys of replacements with their associated vlaues.
// Keys should include the leading backslash, and all matches are greedy. Unmatched macro arguments are ignored.
// p.s., you need to insert entries for \(), \+, \@ yourself.
using MacroReplacements = std::unordered_map<std::string, std::string, bts::cs_hash, bts::cs_eq>;
std::string replace_macro_arguments(std::string_view input, MacroReplacements replacements);
} // namespace pepp::tc
