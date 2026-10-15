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
#include <cstddef>
#include <stdexcept>
#include "../../integers.h"
#include "core/formats/dwarf/enums.hpp"
#include "core/formats/dwarf/forms.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/math/bitmanip/span.hpp"
namespace pepp::bts {

#pragma pack(push, 1)
// Header of a full compilation unit, per Section 7.5.1.1 items 1-5.
template <ElfEndian E> struct PackedCompileUnitHeader {
  // Bytes following this field to the end of the unit.
  U32<E> unit_length;
  U16<E> version;
  u8 unit_type;
  u8 address_size;
  U32<E> debug_abbrev_offset;
};
#pragma pack(pop)
static_assert(sizeof(PackedCompileUnitHeader<ElfEndian::le>) == 12);

// One attribute specification of an abbreviation, per Section 7.5.4.
// Serialized as a LEB128 pair, so this struct does not appear in the specification.
struct AttributeSpecification {
  AttributeNames name;
  AttributeForms form;
};

// One abbreviation declaration, per Section 7.5.3.
// Serialized as a LEB128 pair, so this struct does not appear in the specification.
struct Abbreviation {
  u64 code;
  Tags tag;
  ChildrenDetermination children;
  bits::span<const AttributeSpecification> attributes;
};

// Appends an abbreviations table, ending in a 0 code, and returns its offset in `out`.
// DW_FORM_implicit_const is not supported, since its specification carries a third part.
// Per: Section 7.5.3 and 4.
inline size_t write_abbreviations(AStorage &out, bits::span<const Abbreviation> abbreviations) {
  const auto ret = out.size();
  for (const auto &[code, tag, children, attributes] : abbreviations) {
    // Section 7.5.2: code 0 is reserved for null entries.
    if (code == 0) throw std::logic_error("write_abbreviations: abbreviation code 0 is reserved");
    out.append_uleb(code);
    out.append_uleb(bits::to_underlying(tag));
    out.append(bits::to_underlying(children));
    for (const auto &[name, form] : attributes) {
      if (form == AttributeForms::DW_FORM_implicit_const)
        throw std::logic_error("write_abbreviations: DW_FORM_implicit_const is not supported");
      out.append_uleb(bits::to_underlying(name));
      out.append_uleb(bits::to_underlying(form));
    }
    out.append_uleb(0);
    out.append_uleb(0);
  }
  out.append_uleb(0);
  return ret;
}

// Streams one compilation unit into .debug_info: the header on construction, entries as they are written, and
// unit_length when finished. Nothing else may be appended to `out` until finish() returns.
template <ElfEndian E> class CompileUnitWriter {
public:
  CompileUnitWriter(AStorage &out, u8 address_size, u32 debug_abbrev_offset);

  // Section 7.5.1.1 item 2.
  static constexpr u16 version() noexcept { return 5; }
  // Section 7.5.2 an entry is its abbreviation code, then its attribute values in the abbreviation's order.
  void entry(u64 code);
  // Append a single null
  void end_children();
  // Appends one attribute value. Returns its offset in `out`, so that a caller can relocate it.
  size_t value(AttributeForms form, const FormValue &value);
  // Appends a DW_FORM_addr value, which is address_size bytes (Section 7.5.5). Returns its offset in `out`.
  size_t address(u64 address);
  // Offset in `out` of the header's debug_abbrev_offset, so that a caller can relocate it.
  size_t abbrev_offset_field() const noexcept;
  // Patches unit_length. Returns the offset of the unit within `out`.
  size_t finish();

private:
  AStorage &_out;
  u8 _address_size;
  size_t _unit;
};

template <ElfEndian E>
CompileUnitWriter<E>::CompileUnitWriter(AStorage &out, u8 address_size, u32 debug_abbrev_offset)
    : _out(out), _address_size(address_size) {
  PackedCompileUnitHeader<E> packed{};
  packed.version = version();
  packed.unit_type = bits::to_underlying(UnitTypes::DW_UT_compile);
  packed.address_size = address_size;
  packed.debug_abbrev_offset = debug_abbrev_offset;
  // unit_length depends on the entries still to come, so finish() patches it.
  _unit = _out.append(packed);
}

template <ElfEndian E> void CompileUnitWriter<E>::entry(u64 code) {
  if (code == 0) throw std::logic_error("CompileUnitWriter: use end_children() for a null entry");
  _out.append_uleb(code);
}

template <ElfEndian E> void CompileUnitWriter<E>::end_children() { _out.append_uleb(0); }

template <ElfEndian E> size_t CompileUnitWriter<E>::value(AttributeForms form, const FormValue &value) {
  const auto ret = _out.size();
  write_form<E>(_out, form, value);
  return ret;
}

template <ElfEndian E> size_t CompileUnitWriter<E>::address(u64 address) {
  const auto ret = _out.size();
  detail::write_address<E>(_out, _address_size, address);
  return ret;
}

template <ElfEndian E> size_t CompileUnitWriter<E>::abbrev_offset_field() const noexcept {
  return _unit + offsetof(PackedCompileUnitHeader<E>, debug_abbrev_offset);
}

template <ElfEndian E> size_t CompileUnitWriter<E>::finish() {
  // Section 7.5.1.1 item 1: the length does not include the length field itself.
  constexpr auto length_end = offsetof(PackedCompileUnitHeader<E>, version);
  _out.set(_unit + offsetof(PackedCompileUnitHeader<E>, unit_length),
           U32<E>(static_cast<u32>(_out.size() - _unit - length_end)));
  return _unit;
}

} // namespace pepp::bts
