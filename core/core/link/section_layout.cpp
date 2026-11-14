/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "core/link/section_layout.hpp"
#include <algorithm>
#include <string>
#include "core/link/script_value.hpp"
#include "core/math/bitmanip/log2.hpp"
#include "fmt/format.h"

namespace {
namespace ld = pepp::tc::ldscript;
using pepp::link::LinkCode;
using pepp::link::ScriptContext;
using pepp::link::ScriptMessage;
using pepp::link::ScriptValue;
using pepp::link::Severity;

void apply_assignment(pepp::link::Diagnostics &diagnostics, ScriptContext &context, const ld::Assignment &assignment) {
  // TODO: Symbol values are not recorded yet. PROVIDE only takes effect for referenced symbols.
  using enum ld::Assignment::Kind;
  if (assignment.symbol != "." && (assignment.kind == Provide || assignment.kind == ProvideHidden)) return;
  const auto where = assignment.source_interval;
  auto value = evaluate(assignment.value, context);
  if (!value) {
    const auto message = value.error().message();
    return diagnostics.add({Severity::Error, LinkCode::BadExpression, ScriptMessage{{}, where, message}});
  }
  if (assignment.compound) {
    const auto combined = combine(*assignment.compound, location_counter(context), *value, context);
    if (!combined) {
      const auto message = pepp::tc::expr::to_string(combined.error());
      return diagnostics.add({Severity::Error, LinkCode::BadExpression, ScriptMessage{{}, where, message}});
    }
    value = *combined;
  }
  if (assignment.symbol != ".") return;

  if (!context.current) {
    context.dot = absolute(*value, context);
    return;
  }
  // Within an output section, a number is an offset from the section's address.
  const u64 start = context.address_of(context.current);
  const u64 next = value->kind == ScriptValue::Kind::Number ? start + value->value : absolute(*value, context);
  if (next < context.dot) {
    const auto message =
        fmt::format("cannot move location counter backwards (from {:#x} to {:#x})", context.dot, next);
    return diagnostics.add({Severity::Error, LinkCode::LocationCounterBackwards, ScriptMessage{{}, where, message}});
  }
  context.dot = next;
}

// Place an output section and its members in memory.
void place_output(pepp::link::Diagnostics &diagnostics, ScriptContext &context,
                  pepp::link::SectionPlacements &placements, std::span<const u32> members,
                  const std::function<pepp::link::SectionExtent(pepp::bts::InputElfSectionHandle)> &extent_of,
                  ld::OutputSectionDescription &output) {
  using enum pepp::bts::SectionTypes;
  const auto is_assignment_to = [](const ld::OutputSectionDescription::Command &command, bool dot) {
    const auto *assignment = std::get_if<ld::Assignment>(&command);
    return assignment && (assignment->symbol == ".") == dot;
  };
  const bool assigns_dot =
      std::ranges::any_of(output.commands, [&](const auto &it) { return is_assignment_to(it, true); });
  const bool defines_symbols =
      std::ranges::any_of(output.commands, [&](const auto &it) { return is_assignment_to(it, false); });
  // An output section whose members are all size 0 is discared, and its address is ignored
  // unless it defines symbols.
  if (!output.has_contents && !assigns_dot) {
    output.empty = true;
    if (!defines_symbols) return;
  }

  // Non-allocated output sections are not part of the program's memory, so they start at 0 and do not move `.`.
  const auto alloc = static_cast<u64>(pepp::bts::SectionFlags::SHF_ALLOC);
  const bool allocated = output.type == SHT_NULL || (output.flags & alloc) != 0;
  const u64 saved_dot = context.dot;
  if (!allocated) context.dot = 0;
  output.vma = bits::align_up(context.dot, output.alignment);
  if (output.address) {
    if (const auto address = evaluate(*output.address, context)) {
      output.vma = absolute(*address, context);
      // ld does not round an explicit address up for alignment. Lower the section's alignment to the largest power of
      // 2 that divides the address. Later passes can complain about under-alignment.
      if (output.vma % output.alignment != 0) output.alignment = output.vma & (~output.vma + 1);
    } else {
      const ScriptMessage at{{}, output.source_interval, address.error().message()};
      diagnostics.add({Severity::Error, LinkCode::BadExpression, at});
    }
  }

  context.dot = output.vma;
  context.current = output.id;
  auto next = members.begin();
  for (const auto &command : output.commands) {
    if (const auto *assignment = std::get_if<ld::Assignment>(&command))
      apply_assignment(diagnostics, context, *assignment);
    else {
      // Members are sorted by description, so this description's members come next.
      const auto id = std::get<ld::InputSectionDescription>(command).id;
      for (; next != members.end(); ++next) {
        if (placements.assignments()[*next].description != id) break;
        const auto extent = extent_of(placements.sections().handle(*next));
        context.dot = bits::align_up(context.dot, extent.alignment);
        placements.set_offset(*next, context.dot - output.vma);
        context.dot += extent.size;
      }
    }
  }
  output.size = context.dot - output.vma;
  context.current = {};
  if (!allocated) context.dot = saved_dot;
}
} // namespace

std::vector<ld::OutputSectionDescription *> pepp::link::output_sections(tc::ldscript::Sections &sections) {
  std::vector<ld::OutputSectionDescription *> ret(1, nullptr);
  for (auto &command : sections.commands)
    if (auto *output = std::get_if<ld::OutputSectionDescription>(&command)) {
      if (output->id.value >= ret.size()) ret.resize(output->id.value + 1, nullptr);
      ret[output->id.value] = output;
    }
  return ret;
}

void pepp::link::lay_out_sections(Diagnostics &diagnostics, SectionPlacements &placements,
                                  const std::function<SectionExtent(InputElfSectionHandle)> &extent_of,
                                  tc::ldscript::Sections &sections) {
  const auto outputs = output_sections(sections);
  const auto members = placements.members();
  ScriptContext context{.address_of = [&outputs](OutputSectionID id) { return outputs.at(id.value)->vma; }};
  for (auto &command : sections.commands) {
    if (const auto *assignment = std::get_if<ld::Assignment>(&command))
      apply_assignment(diagnostics, context, *assignment);
    else {
      auto &output = std::get<ld::OutputSectionDescription>(command);
      place_output(diagnostics, context, placements, members.members(output.id), extent_of, output);
    }
  }
}
