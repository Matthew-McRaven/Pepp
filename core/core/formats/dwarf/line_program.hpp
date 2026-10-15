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

#pragma once
#include <expected>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include "../../integers.h"
#include "core/formats/dwarf/cursor.hpp"
#include "core/formats/dwarf/enums.hpp"
#include "core/formats/dwarf/line_info.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"
namespace pepp::bts {

// The line number state machine's registers, which are also the columns of a row in the line number matrix.
// Per: Section 6.2.2, Table 6.3.
struct DebugLineState {
  u64 address = 0;
  // Index of an operation within a VLIW instruction. Always 0 for non-VLIW architectures.
  u64 op_index = 0;
  u64 file = 1;
  // Lines are numbered from 1. 0 means the instruction cannot be attributed to a source line.
  u64 line = 1;
  // Columns are numbered from 1. 0 means the statement begins at the left edge of the line.
  u64 column = 0;
  bool is_stmt = false;
  bool basic_block = false;
  // This row's address is the first byte after a sequence. Its other fields are not meaningful.
  bool end_sequence = false;
  bool prologue_end = false;
  bool epilogue_begin = false;
  u64 isa = 0;
  u64 discriminator = 0;

  DebugLineState() = default;
  // Default-init values are already correct except for default_is_stmt(), see Table 6.4.
  explicit DebugLineState(const LineProgramHeader &header) noexcept : is_stmt(header.default_is_stmt) {}

  friend void swap(DebugLineState &lhs, DebugLineState &rhs) noexcept {
    using std::swap;
    swap(lhs.address, rhs.address);
    swap(lhs.op_index, rhs.op_index);
    swap(lhs.file, rhs.file);
    swap(lhs.line, rhs.line);
    swap(lhs.column, rhs.column);
    swap(lhs.is_stmt, rhs.is_stmt);
    swap(lhs.basic_block, rhs.basic_block);
    swap(lhs.end_sequence, rhs.end_sequence);
    swap(lhs.prologue_end, rhs.prologue_end);
    swap(lhs.epilogue_begin, rhs.epilogue_begin);
    swap(lhs.isa, rhs.isa);
    swap(lhs.discriminator, rhs.discriminator);
  }
};

// Construct a single line program into `out` in a streaming fashion. The header is written on construction and opcodes
// are emitted as row() is called. Out may have no concurrent writers until finish() is called.
// maximum_operations_per_instruction must be 1, so op_index is always 0.
template <ElfEndian E> class LineProgramWriter {
public:
  LineProgramWriter(AStorage &out, const LineProgramHeader &header);

  // Appends a row for the instructions occupying [address, address + length).
  // Section 6.2.5 only lets addresses increase within a sequence, so a lower address first ends the open sequence.
  void row(u64 address, u64 length, u64 file, u64 line);
  // Ends the open sequence, if any, at the end of its highest row.
  void end_sequence();
  // Ends any open sequence and patches unit_length. Returns the offset of the unit within `out`.
  size_t finish();

private:
  void standard(StandardOpcodes opcode);
  // Writes everything before an extended opcode's operands, which the caller appends next.
  void extended(ExtendedOpcodes opcode, u64 operands_size);
  void set_address(u64 address);
  void advance_to(u64 address);
  std::optional<u8> special_opcode(i64 line_advance, u64 operation_advance) const noexcept;

  AStorage &_out;
  DebugLineState _initial, _state;
  u8 _address_size, _minimum_instruction_length;
  i8 _line_base;
  u8 _line_range, _opcode_base;
  // Section 6.2.5.2: DW_LNS_const_add_pc advances the address as special opcode 255 does.
  u64 _const_add_pc_advance;
  size_t _unit;
  bool _open = false;
  u64 _sequence_end = 0;
};

// Executes `unit.program`, calling `on_row(const DebugLineState &)` each time a row is appended to the matrix.
// Per: DWARF 5 Section 6.2.5.
template <ElfEndian E, typename OnRow>
std::expected<void, std::string> run_line_program(const LineProgramUnit &unit, OnRow &&on_row);

/*
 * Implementations
 */

namespace detail {
template <ElfEndian E> void write_address(AStorage &out, u8 size, u64 address) {
  switch (size) {
  case 1: out.append(static_cast<u8>(address)); break;
  case 2: out.append(U16<E>(static_cast<u16>(address))); break;
  case 4: out.append(U32<E>(static_cast<u32>(address))); break;
  case 8: out.append(U64<E>(address)); break;
  default: throw std::logic_error("write_address: unsupported address_size");
  }
}

template <ElfEndian E> u64 read_address(DwarfCursor &c, u8 size) {
  switch (size) {
  case 1: return c.fixed<u8>();
  case 2: return c.fixed<U16<E>>();
  case 4: return c.fixed<U32<E>>();
  case 8: return c.fixed<U64<E>>();
  default: c.fail("unsupported address_size"); return 0;
  }
}
} // namespace detail

template <ElfEndian E>
LineProgramWriter<E>::LineProgramWriter(AStorage &out, const LineProgramHeader &header)
    : _out(out), _initial(header), _state(header), _address_size(header.address_size),
      _minimum_instruction_length(header.minimum_instruction_length), _line_base(header.line_base),
      _line_range(header.line_range), _opcode_base(header.opcode_base()) {
  if (header.maximum_operations_per_instruction != 1)
    throw std::logic_error("LineProgramWriter: maximum_operations_per_instruction must be 1");
  else if (_minimum_instruction_length == 0)
    throw std::logic_error("LineProgramWriter: minimum_instruction_length must be nonzero");
  else if (_line_range == 0) throw std::logic_error("LineProgramWriter: line_range must be nonzero");
  // Lets every row end in a special opcode once standard opcodes have moved the line and address close enough.
  else if (_line_base > 0 || _line_base + _line_range - 1 < 0 || _opcode_base + _line_range - 1 > 255)
    throw std::logic_error("LineProgramWriter: a special opcode must exist for every in-range line increment");
  _const_add_pc_advance = (255 - _opcode_base) / _line_range;
  // unit_length depends on the opcodes still to come, so finish() patches it.
  _unit = write_line_program_header<E>(out, header, 0);
}

template <ElfEndian E> void LineProgramWriter<E>::row(u64 address, u64 length, u64 file, u64 line) {
  using enum StandardOpcodes;
  if (_open && address < _state.address) end_sequence();
  if (!_open) {
    set_address(address);
    _open = true, _sequence_end = address;
  } else if ((address - _state.address) % _minimum_instruction_length != 0) set_address(address);
  _sequence_end = std::max(_sequence_end, address + length);

  if (file != _state.file) {
    standard(DW_LNS_set_file);
    _out.append_uleb(file);
    _state.file = file;
  }

  // Section 6.2.5.1: one special opcode advances the address and line, then appends a row.
  u64 operation_advance = (address - _state.address) / _minimum_instruction_length;
  i64 line_advance = static_cast<i64>(line - _state.line);
  // Section 6.2.5.1: line increments outside [line_base, line_base + line_range - 1] need a standard opcode.
  if (line_advance < _line_base || line_advance > _line_base + _line_range - 1) {
    standard(DW_LNS_advance_line);
    _out.append_sleb(line_advance);
    line_advance = 0;
  }
  // Move the address with standard opcodes until the rest fits in a special opcode.
  if (!special_opcode(line_advance, operation_advance)) {
    if (operation_advance >= _const_add_pc_advance &&
        special_opcode(line_advance, operation_advance - _const_add_pc_advance)) {
      standard(DW_LNS_const_add_pc);
      operation_advance -= _const_add_pc_advance;
    } else {
      standard(DW_LNS_advance_pc);
      _out.append_uleb(operation_advance);
      operation_advance = 0;
    }
  }
  // The constructor guarantees a special opcode exists once operation_advance is 0.
  _out.append(*special_opcode(line_advance, operation_advance));
  _state.address = address, _state.line = line;
}

template <ElfEndian E> void LineProgramWriter<E>::end_sequence() {
  if (!_open) return;
  // Section 6.2.5.3: the end_sequence row's address is the byte after the sequence's last instruction.
  advance_to(_sequence_end);
  extended(ExtendedOpcodes::DW_LNE_end_sequence, 0);
  // Section 6.2.5.3: DW_LNE_end_sequence resets the registers to their initial values.
  _state = _initial, _open = false;
}

template <ElfEndian E> size_t LineProgramWriter<E>::finish() {
  end_sequence();
  // Patch the unit_length field to include the size of the program/body
  _out.set(_unit + offsetof(PackedLineProgramHeader<E>, unit_length),
           U32<E>(static_cast<u32>(_out.size() - _unit - line_unit_length_end)));
  return _unit;
}

template <ElfEndian E>
std::optional<u8> LineProgramWriter<E>::special_opcode(i64 line_advance, u64 operation_advance) const noexcept {
  // Section 6.2.5.1: opcode = (line increment - line_base) + (line_range * operation advance) + opcode_base, and a
  // standard opcode is needed instead when the line increment is out of range or the opcode is greater than 255.
  if (line_advance < _line_base || line_advance > _line_base + _line_range - 1 || operation_advance > 255) return {};
  const u64 opcode = static_cast<u64>(line_advance - _line_base) + _line_range * operation_advance + _opcode_base;
  if (opcode > 255) return {};
  return static_cast<u8>(opcode);
}

template <ElfEndian E> void LineProgramWriter<E>::standard(StandardOpcodes opcode) {
  _out.append(bits::to_underlying(opcode));
}

template <ElfEndian E> void LineProgramWriter<E>::extended(ExtendedOpcodes opcode, u64 operands_size) {
  // Section 6.2.3: a zero byte, then a ULEB128 count of the bytes in the instruction itself (the opcode and its
  // operands), then the opcode.
  _out.append(u8{0});
  _out.append_uleb(sizeof(opcode) + operands_size);
  _out.append(bits::to_underlying(opcode));
}

template <ElfEndian E> void LineProgramWriter<E>::set_address(u64 address) {
  extended(ExtendedOpcodes::DW_LNE_set_address, _address_size);
  detail::write_address<E>(_out, _address_size, address);
  _state.address = address;
}

template <ElfEndian E> void LineProgramWriter<E>::advance_to(u64 address) {
  const u64 delta = address - _state.address;
  if (delta == 0) return;
  // DW_LNS_advance_pc argument is multiplied by minimum_instruction_length, so other deltas need an absolute address.
  if (delta % _minimum_instruction_length != 0) return set_address(address);
  standard(StandardOpcodes::DW_LNS_advance_pc);
  _out.append_uleb(delta / _minimum_instruction_length);
  _state.address = address;
}

template <ElfEndian E, typename OnRow>
std::expected<void, std::string> run_line_program(const LineProgramUnit &unit, OnRow &&on_row) {
  using enum StandardOpcodes;
  using enum ExtendedOpcodes;
  const auto &header = unit.header;
  if (header.maximum_operations_per_instruction == 0)
    return std::unexpected("maximum_operations_per_instruction must be nonzero");
  const u8 opcode_base = header.opcode_base();
  DebugLineState state(header);

  // Section 6.2.5.1: how special opcodes, DW_LNS_advance_pc, and DW_LNS_const_add_pc move the operation pointer.
  const auto advance = [&](u64 operation_advance) {
    const u64 operations = state.op_index + operation_advance;
    state.address += header.minimum_instruction_length * (operations / header.maximum_operations_per_instruction);
    state.op_index = operations % header.maximum_operations_per_instruction;
  };
  // Sections 6.2.5.1 and 6.2.5.2 (DW_LNS_copy): these registers reset after a row is appended.
  const auto append_row = [&] {
    on_row(static_cast<const DebugLineState &>(state));
    state.discriminator = 0;
    state.basic_block = state.prologue_end = state.epilogue_begin = false;
  };

  DwarfCursor c{unit.program};
  while (c.ok() && c.remaining() > 0) {
    const u8 opcode = c.fixed<u8>();
    if (opcode >= opcode_base) {
      // Section 6.2.5.1.
      const u8 adjusted = opcode - opcode_base;
      advance(adjusted / header.line_range);
      state.line += static_cast<i64>(header.line_base) + adjusted % header.line_range;
      append_row();
    } else if (opcode == 0) {
      // Section 6.2.3: a ULEB128 length covers the extended opcode and its operands. Unknown opcodes are skipped.
      DwarfCursor body{c.bytes(c.uleb())};
      if (!c.ok()) break;
      const auto extended = ExtendedOpcodes(body.fixed<u8>());
      if (!body.ok()) return std::unexpected("extended opcode has no opcode byte");
      switch (extended) {
      // Section 6.2.5.3.
      case DW_LNE_end_sequence:
        state.end_sequence = true;
        on_row(static_cast<const DebugLineState &>(state));
        state = DebugLineState(header);
        break;
      case DW_LNE_set_address:
        state.address = detail::read_address<E>(body, header.address_size);
        state.op_index = 0;
        break;
      case DW_LNE_set_discriminator: state.discriminator = body.uleb(); break;
      default: continue;
      }
      // A known opcode's operands must fill the length exactly, or its operand sizes disagree with the producer's.
      if (!body.ok()) return std::unexpected(body.error);
      else if (body.remaining() != 0) return std::unexpected("extended opcode is longer than its operands");
    } else {
      // Section 6.2.5.2.
      switch (StandardOpcodes(opcode)) {
      case DW_LNS_copy: append_row(); break;
      case DW_LNS_advance_pc: advance(c.uleb()); break;
      case DW_LNS_advance_line: state.line += c.sleb(); break;
      case DW_LNS_set_file: state.file = c.uleb(); break;
      case DW_LNS_set_column: state.column = c.uleb(); break;
      case DW_LNS_negate_stmt: state.is_stmt = !state.is_stmt; break;
      case DW_LNS_set_basic_block: state.basic_block = true; break;
      case DW_LNS_const_add_pc: advance((255 - opcode_base) / header.line_range); break;
      case DW_LNS_fixed_advance_pc:
        state.address += c.fixed<U16<E>>();
        state.op_index = 0;
        break;
      case DW_LNS_set_prologue_end: state.prologue_end = true; break;
      case DW_LNS_set_epilogue_begin: state.epilogue_begin = true; break;
      case DW_LNS_set_isa: state.isa = c.uleb(); break;
      default:
        // Section 6.2.4 item 12: unknown standard opcodes are skipped using their LEB128 operand counts.
        for (u8 it = 0; it < header.standard_opcode_lengths[opcode - 1]; ++it) c.uleb();
        break;
      }
    }
  }
  if (!c.ok()) return std::unexpected(c.error);
  return {};
}

} // namespace pepp::bts
