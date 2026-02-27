#pragma once

#include <fmt/ranges.h>
#include <sstream>
#include "core/langs/ucode/pep_parser.hpp"

namespace pepp::tc::ir {

template <typename registers> std::string to_string(const Test<registers> &test) {
  if (std::holds_alternative<MemTest>(test)) return std::get<MemTest>(test);
  else if (std::holds_alternative<RegisterTest<registers>>(test)) return std::get<RegisterTest<registers>>(test);
  else if (std::holds_alternative<CSRTest<registers>>(test)) return std::get<CSRTest<registers>>(test);
  else return "";
}

// Given an assembled line, produce the cannonical formatted representation of that line.
template <typename uarch, typename registers> std::string format(const Line<uarch, registers> &line) {
  using Type = Line<uarch, registers>::Type;
  switch (line.type) {
  case Type::Pre: [[fallthrough]];
  case Type::Post: {
    static const char *type_str[] = {"UnitPre", "UnitPost"};
    int i = line.type == Type::Pre ? 0 : 1;
    std::vector<std::string> tests;
    for (const auto &test : line.tests) tests.push_back(to_string(test));
    return fmt::format("{}: {}", type_str[i], fmt::join(tests.begin(), tests.end(), ", "));
  }
  case Type::Code: {
    std::string symbolDecl;
    if (line.symbolDecl.has_value()) symbolDecl = *line.symbolDecl + ": ";
    std::string _signals = line.controls.toString();
    return fmt::format("{}{}{}", symbolDecl, _signals, line.comment.has_value() ? *line.comment : "");
  }
  default: throw std::logic_error("Unrecognized line type!");
  }
}

// Given a program, produce the cannonical formatted representation of that line.
enum class FormatStyle {
  SourceStyle,  // Without cycle numbers
  ListingStyle, // With cycle numbers
};
template <typename uarch, typename registers>
std::string format(const parse::ParseResult<uarch, registers> &result, FormatStyle style = FormatStyle::SourceStyle) {
  std::ostringstream out;
  for (u32 it = 0; it < result.program.size(); it++) {
    const auto &line = result.program[it];
    // Addresses start at 0, but cycle numbers start at 1.
    if (style == FormatStyle::ListingStyle && parse::is_code_line(line)) out << fmt::format("{}. ", line.address + 1);
    out << format(line) << "\n";
  }
  return out.str();
}

} // namespace pepp::tc::ir
