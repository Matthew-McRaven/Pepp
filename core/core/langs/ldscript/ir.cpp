/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/ldscript/ir.hpp"
// algorithm must be before array and iterator for ranges to work
#include <algorithm>
// algorithm must be before array and iterator for ranges to work
#include <array>
#include <iterator>
#include <ranges>
#include "core/langs/ldscript/lexer.hpp"
#include "fmt/format.h"
#include "fmt/ranges.h"

namespace {
using namespace pepp::tc::ldscript;
using pepp::bts::ElfClass;
using pepp::bts::ElfEncoding;
using pepp::bts::ElfMachineType;

struct NamedFormat {
  std::string_view name;
  OutputFormat::Format format;
};
// BFD target names for RISC-V (from GNU) and custom names for Pep. Pep is big-endian.
constexpr std::array formats{
    NamedFormat{"elf32-littleriscv", {ElfClass::ELFCLASS32, ElfEncoding::ELFDATA2LSB, ElfMachineType::EM_RISCV}},
    NamedFormat{"elf32-bigriscv", {ElfClass::ELFCLASS32, ElfEncoding::ELFDATA2MSB, ElfMachineType::EM_RISCV}},
    NamedFormat{"elf64-littleriscv", {ElfClass::ELFCLASS64, ElfEncoding::ELFDATA2LSB, ElfMachineType::EM_RISCV}},
    NamedFormat{"elf64-bigriscv", {ElfClass::ELFCLASS64, ElfEncoding::ELFDATA2MSB, ElfMachineType::EM_RISCV}},
    NamedFormat{"elf32-pep8", {ElfClass::ELFCLASS32, ElfEncoding::ELFDATA2MSB, ElfMachineType::EM_PEP8}},
    NamedFormat{"elf32-pep9", {ElfClass::ELFCLASS32, ElfEncoding::ELFDATA2MSB, ElfMachineType::EM_PEP9}},
    NamedFormat{"elf32-pep10", {ElfClass::ELFCLASS32, ElfEncoding::ELFDATA2MSB, ElfMachineType::EM_PEP10}},
};

struct NamedAssignment {
  std::string_view keyword;
  Assignment::Kind kind;
};
constexpr std::array assignments{
    NamedAssignment{"PROVIDE", Assignment::Kind::Provide},
    NamedAssignment{"PROVIDE_HIDDEN", Assignment::Kind::ProvideHidden},
    NamedAssignment{"HIDDEN", Assignment::Kind::Hidden},
};

struct NamedArchitecture {
  std::string_view name;
  ElfMachineType machine;
};
constexpr std::array architectures{
    NamedArchitecture{"riscv", ElfMachineType::EM_RISCV},
    NamedArchitecture{"pep8", ElfMachineType::EM_PEP8},
    NamedArchitecture{"pep9", ElfMachineType::EM_PEP9},
    NamedArchitecture{"pep10", ElfMachineType::EM_PEP10},
};

std::string name(std::string_view text) {
  return is_unquoted_identifier(text) ? std::string(text) : fmt::format("\"{}\"", text);
}

std::string to_string(const pepp::tc::expr::Tree &expression) { return pepp::tc::expr::to_infix(expression); }

std::string to_string(const Assignment &assignment) {
  const auto op = assignment.compound ? fmt::format("{}=", pepp::tc::expr::to_string(*assignment.compound)) : "=";
  const auto plain = fmt::format("{} {} {}", name(assignment.symbol), op, to_string(assignment.value));
  const auto keyword = pepp::tc::ldscript::to_string(assignment.kind);
  return keyword.empty() ? plain + ";" : fmt::format("{}({});", keyword, plain);
}

std::string to_string(const FilePattern &file) {
  using enum FilePattern::Match;
  if (file.match == WholeArchive) return name(file.pattern) + ':';
  else if (file.match == NotInArchive) return ':' + name(file.pattern);
  return name(file.pattern);
}

std::string to_string(const InputSectionDescription &input) {
  const auto description =
      fmt::format("{}({})", to_string(input.file), fmt::join(input.sections | std::views::transform(name), " "));
  return input.keep ? fmt::format("KEEP({})", description) : description;
}

std::string to_string(const InputFiles &input) {
  return fmt::format("{}({})", input.kind == InputFiles::Kind::Group ? "GROUP" : "INPUT",
                     fmt::join(input.files | std::views::transform(name), " "));
}

// Always quote format names, since ld --verbose quotes them.
std::string to_string(const OutputFormat &output) {
  const auto quoted = [](const OutputFormat::Format &format) {
    return fmt::format("\"{}\"", pepp::tc::ldscript::to_string(format));
  };
  if (output.big.machine == ElfMachineType::EM_NONE) return fmt::format("OUTPUT_FORMAT({})", quoted(output.format));
  return fmt::format("OUTPUT_FORMAT({}, {}, {})", quoted(output.format), quoted(output.big), quoted(output.little));
}

std::string to_string(const OutputArch &output) {
  return fmt::format("OUTPUT_ARCH({})", architecture_name(output.machine));
}

std::string to_string(const OutputSectionDescription &output);

// Print each item on its own line after indent. Nested items are already indented to their depth.
template <typename Items> std::string lines(const Items &items, std::string_view indent) {
  const auto line = [&](const auto &item) {
    return std::visit([&](const auto &it) { return fmt::format("{}{}\n", indent, to_string(it)); }, item);
  };
  return fmt::to_string(fmt::join(items | std::views::transform(line), ""));
}

std::string to_string(const OutputSectionDescription &output) {
  const auto address = output.address ? " " + to_string(*output.address) : "";
  return fmt::format("{}{} :\n  {{\n{}  }}", name(output.name), address, lines(output.items, "    "));
}

std::string to_string(const Entry &entry) { return fmt::format("ENTRY({})", name(entry.symbol)); }

std::string to_string(const Sections &sections) {
  return fmt::format("SECTIONS\n{{\n{}}}", lines(sections.items, "  "));
}
} // namespace

std::optional<pepp::tc::ldscript::OutputFormat::Format> pepp::tc::ldscript::output_format(std::string_view name) {
  const auto it = std::ranges::find(formats, name, &NamedFormat::name);
  if (it == formats.end()) return std::nullopt;
  return it->format;
}

std::string_view pepp::tc::ldscript::to_string(const OutputFormat::Format &format) {
  const auto it = std::ranges::find(formats, format, &NamedFormat::format);
  return it == formats.end() ? std::string_view{} : it->name;
}

std::string_view pepp::tc::ldscript::to_string(Assignment::Kind kind) {
  const auto it = std::ranges::find(assignments, kind, &NamedAssignment::kind);
  return it == assignments.end() ? std::string_view{} : it->keyword;
}

std::optional<pepp::tc::ldscript::Assignment::Kind> pepp::tc::ldscript::assignment_kind(std::string_view keyword) {
  const auto it = std::ranges::find(assignments, keyword, &NamedAssignment::keyword);
  if (it == assignments.end()) return std::nullopt;
  return it->kind;
}

std::optional<pepp::bts::ElfMachineType> pepp::tc::ldscript::architecture(std::string_view name) {
  const auto it = std::ranges::find(architectures, name, &NamedArchitecture::name);
  if (it == architectures.end()) return std::nullopt;
  return it->machine;
}

std::string_view pepp::tc::ldscript::architecture_name(bts::ElfMachineType machine) {
  const auto it = std::ranges::find(architectures, machine, &NamedArchitecture::machine);
  return it == architectures.end() ? std::string_view{} : it->name;
}

std::string pepp::tc::ldscript::to_string(const Script &script) {
  std::string ret;
  for (const auto &command : script.commands)
    std::visit([&](const auto &it) { ret += ::to_string(it) + "\n"; }, command);
  return ret;
}

std::vector<const pepp::tc::ldscript::OutputSectionDescription *>
pepp::tc::ldscript::output_sections(const Script &script) {
  const auto is_sections = [](const Script::Command &command) { return std::holds_alternative<Sections>(command); };
  const auto items = [](const Script::Command &command) -> const auto & { return std::get<Sections>(command).items; };
  const auto is_statement = [](const Sections::Item &item) {
    return std::holds_alternative<OutputSectionDescription>(item);
  };
  const auto statement = [](const Sections::Item &item) { return &std::get<OutputSectionDescription>(item); };
  std::vector<const OutputSectionDescription *> ret;
  // For all SECTION directives, extract their items. For OutputSectionDescription in items, extract the description.
  std::ranges::copy(script.commands | std::views::filter(is_sections) | std::views::transform(items) |
                        std::views::join | std::views::filter(is_statement) | std::views::transform(statement),
                    std::back_inserter(ret));
  return ret;
}
