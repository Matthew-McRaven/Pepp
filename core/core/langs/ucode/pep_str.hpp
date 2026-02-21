#pragma once

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
  std::string symbolDecl;
  if (line.symbolDecl.has_value()) symbolDecl = *line.symbolDecl + ": ";
  std::string _signals = line.controls.toString();
  return fmt::format("{}{}{}", symbolDecl, _signals, line.comment.has_value() ? *line.comment : "");
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
    if (style == FormatStyle::ListingStyle && parse::is_code_line(line)) out << fmt::format("{}.", line.address);
    out << format(line) << "\n";
  }
  return out.str();
}

} // namespace pepp::tc::ir
