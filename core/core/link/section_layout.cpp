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

// Place an output section and its members in memory while recording them in layout.
void place_output(pepp::link::Diagnostics &diagnostics, ScriptContext &context, pepp::link::SectionLayout &layout,
                  const pepp::link::SectionPlacements &placements,
                  std::span<const pepp::bts::InputElfSectionHandle> members,
                  const std::function<pepp::link::SectionExtent(pepp::bts::InputElfSectionHandle)> &extent_of,
                  const ld::OutputSectionDescription &output) {
  using enum pepp::bts::SectionTypes;
  auto &range = layout.output(output.id);
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
  const bool empty = std::ranges::all_of(members, [&](auto member) { return extent_of(member).size == 0; });
  if (empty && !assigns_dot) {
    range.empty = true;
    if (!defines_symbols) return;
  }

  for (const auto member : members) range.alignment = std::max(range.alignment, extent_of(member).alignment);
  // Non-allocated output sections are not part of the program's memory, so they start at 0 and do not move `.`.
  const auto alloc = static_cast<u64>(pepp::bts::SectionFlags::SHF_ALLOC);
  const bool allocated = output.type == SHT_NULL || (output.flags & alloc) != 0;
  const u64 saved_dot = context.dot;
  if (!allocated) context.dot = 0;
  range.address = bits::align_up(context.dot, range.alignment);
  if (output.address) {
    if (const auto address = evaluate(*output.address, context)) range.address = absolute(*address, context);
    else {
      const ScriptMessage at{{}, output.source_interval, address.error().message()};
      diagnostics.add({Severity::Error, LinkCode::BadExpression, at});
    }
  }

  context.dot = range.address;
  context.current = output.id;
  auto next = members.begin();
  for (const auto &command : output.commands) {
    if (const auto *assignment = std::get_if<ld::Assignment>(&command))
      apply_assignment(diagnostics, context, *assignment);
    else {
      // Members are sorted by description, so this description's members come next.
      const auto id = std::get<ld::InputSectionDescription>(command).id;
      for (; next != members.end() && placements.assignment_of(*next).description == id; ++next) {
        const auto extent = extent_of(*next);
        context.dot = bits::align_up(context.dot, extent.alignment);
        layout.set_offset(*placements.sections().position(*next), context.dot - range.address);
        context.dot += extent.size;
      }
    }
  }
  range.size = context.dot - range.address;
  context.current = {};
  if (!allocated) context.dot = saved_dot;
}
} // namespace

pepp::link::SectionLayout::SectionLayout(u32 outputs, const SectionPlacements &placements)
    : _outputs(outputs + 1), _offsets(placements.sections().size()) {}

u64 pepp::link::SectionLayout::offset_of(const SectionPlacements &placements, InputElfSectionHandle handle) const {
  const auto position = placements.sections().position(handle);
  if (!position) throw std::out_of_range("SectionLayout: section not covered");
  return _offsets.at(*position);
}

u64 pepp::link::SectionLayout::address_of(const SectionPlacements &placements, InputElfSectionHandle handle) const {
  return output(placements.assignment_of(handle).output).address + offset_of(placements, handle);
}

pepp::link::SectionLayout
pepp::link::lay_out_sections(Diagnostics &diagnostics, const SectionPlacements &placements,
                             const std::function<SectionExtent(InputElfSectionHandle)> &extent_of,
                             const tc::ldscript::Sections &sections) {
  u32 outputs = 0;
  for (const auto &command : sections.commands)
    if (const auto *output = std::get_if<ld::OutputSectionDescription>(&command))
      outputs = std::max(outputs, output->id.value);
  SectionLayout ret(outputs, placements);
  const auto members = placements.members();
  ScriptContext context{.address_of = [&ret](OutputSectionID id) { return ret.output(id).address; }};
  for (const auto &command : sections.commands) {
    if (const auto *assignment = std::get_if<ld::Assignment>(&command))
      apply_assignment(diagnostics, context, *assignment);
    else {
      const auto &output = std::get<ld::OutputSectionDescription>(command);
      place_output(diagnostics, context, ret, placements, members.members(output.id), extent_of, output);
    }
  }
  return ret;
}
