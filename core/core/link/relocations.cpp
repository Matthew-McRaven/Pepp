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
#include "core/link/relocations.hpp"
#include <bit>
#include <expected>
#include "core/arch/riscv/isa/rvi.hpp"
#include "core/formats/elf/enums_sections.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "core/math/bitmanip/log2.hpp"

namespace {
using pepp::link::PairedRelocation;
using pepp::link::RelocationCode;
using pepp::link::ResolvedRelocation;
using Result = std::optional<RelocationCode>;
using RV = pepp::bts::RelocationsRISCV;

// S + A. Must only be called when rel's symbol has an address.
u64 absolute(const ResolvedRelocation &rel) { return *rel.symbol + static_cast<u64>(rel.addend); }

// Write value into a field which must hold it as a signed or unsigned integer.
Result write_absolute(std::span<u8> field, bits::Order order, u64 value) {
  if (!bits::int_fits(value, static_cast<u8>(field.size()))) return RelocationCode::Truncated;
  bits::memcpy_endian(field, order, value);
  return std::nullopt;
}

std::optional<u8> pep_relocation_size(u32 type) {
  using enum pepp::bts::RelocationsPep;
  switch (static_cast<pepp::bts::RelocationsPep>(type)) {
  case R_PEP10_NONE: return 0;
  case R_PEP10_ABS8: return 1;
  case R_PEP10_ABS16: return 2;
  case R_PEP10_ABS32: return 4;
  default: return std::nullopt;
  }
}

// TODO: the rest of the RISC-V psABI's relocations.
std::optional<u8> riscv_relocation_size(RV type) {
  using enum RV;
  switch (type) {
  case R_RISCV_NONE: return 0;
  case R_RISCV_ADD8: [[fallthrough]];
  case R_RISCV_SUB8: return 1;
  case R_RISCV_ADD16: [[fallthrough]];
  case R_RISCV_SUB16: return 2;
  case R_RISCV_32: [[fallthrough]];
  case R_RISCV_ADD32: [[fallthrough]];
  case R_RISCV_SUB32: [[fallthrough]];
  case R_RISCV_32_PCREL: [[fallthrough]];
  case R_RISCV_BRANCH: [[fallthrough]];
  case R_RISCV_JAL: [[fallthrough]];
  case R_RISCV_HI20: [[fallthrough]];
  case R_RISCV_LO12_I: [[fallthrough]];
  case R_RISCV_LO12_S: [[fallthrough]];
  case R_RISCV_PCREL_HI20: [[fallthrough]];
  case R_RISCV_PCREL_LO12_I: [[fallthrough]];
  case R_RISCV_PCREL_LO12_S: return 4;
  case R_RISCV_64: [[fallthrough]];
  case R_RISCV_ADD64: [[fallthrough]];
  case R_RISCV_SUB64: return 8;
  default: return std::nullopt;
  }
}

// Returns the number of bytes written by a relocation or nullopt for unrecognized machines/types.
std::optional<u8> relocation_size(pepp::bts::ElfMachineType machine, u32 type) {
  using enum pepp::bts::ElfMachineType;
  switch (machine) {
  case EM_PEP8: [[fallthrough]];
  case EM_PEP9: [[fallthrough]];
  case EM_PEP10: return pep_relocation_size(type);
  case EM_RISCV: return riscv_relocation_size(static_cast<RV>(type));
  default: return std::nullopt;
  }
}

// Load the instruction containd in field. Apply functor to that loaded instruction before writing it back to field.
template <class Format, class Update> void patch(std::span<u8> field, Update update) {
  // Instructions are always little-endian for rv
  auto insn = std::bit_cast<Format>(bits::memcpy_endian<u32>(field, bits::Order::LittleEndian));
  update(insn);
  bits::memcpy_endian(field, bits::Order::LittleEndian, std::bit_cast<u32>(insn));
}
// Rounds up when bit 11 is set, because the paired lower 12 bits sign-extend.
constexpr u32 high20(i64 value) { return static_cast<u32>((value + 0x800) >> 12); }

// Add delta to the integer in field, wrapping on overflow.
void add_to(std::span<u8> field, bits::Order order, u64 delta) {
  const auto v = bits::memcpy_endian<u64>(field, order) + delta;
  bits::memcpy_endian(field, order, v);
}

// Returns an error if the integer value is not encodable as a branch instruction's immediate.
Result check_jump(i64 value, u8 width) {
  if (bits::signed_bitcount(value) > width) return RelocationCode::Truncated;
  else if (value % 2 != 0) return RelocationCode::Misaligned;
  return std::nullopt;
}

struct Apply {
  pepp::bts::ElfMachineType machine;
  pepp::bts::ElfBits width;
  bits::Order order;
  u64 address;
  std::span<u8> bytes;

  Result operator()(const ResolvedRelocation &rel) const {
    if (const auto field = relocated_field(rel); !field) return field.error();
    else if (field->empty()) return std::nullopt;
    else if (machine == pepp::bts::ElfMachineType::EM_RISCV) return apply_riscv(*field, rel);
    // Only Pep remains, and every Pep relocation writes S + A.
    else return write_absolute(*field, order, absolute(rel));
  }

  // Only an R_RISCV_PCREL_LO12_I or R_RISCV_PCREL_LO12_S has a pair, which is its auipc's R_RISCV_PCREL_HI20.
  Result operator()(const PairedRelocation &paired) const {
    const auto field = relocated_field(paired.rel);
    if (!field) return field.error();
    else if (!paired.pair.symbol) return RelocationCode::NoSymbolAddress;
    const auto low = static_cast<u32>(pc_relative(paired.pair));
    if (static_cast<RV>(paired.rel.type) == RV::R_RISCV_PCREL_LO12_I)
      patch<riscv::InstructionI>(*field, [low](auto &insn) { insn.imm = low; });
    else patch<riscv::InstructionS>(*field, [low](auto &insn) { insn.set_imm(low); });
    return std::nullopt;
  }

private:
  // S + A - P. Must only be called when rel's symbol has an address.
  i64 pc_relative(const ResolvedRelocation &rel) const {
    return static_cast<i64>(absolute(rel) - (address + rel.offset));
  }
  // Returns a pointer to the bytes update by a given relocation.
  // Empty if the relocation writes no bytes or if the relocation cannot be applied.
  std::expected<std::span<u8>, RelocationCode> relocated_field(const ResolvedRelocation &rel) const {
    const auto size = relocation_size(machine, rel.type);
    if (!rel.symbol) return std::unexpected(RelocationCode::NoSymbolAddress);
    else if (!size) return std::unexpected(RelocationCode::UnsupportedType);
    else if (*size == 0) return std::span<u8>{};
    else if (rel.offset > bytes.size() || *size > bytes.size() - rel.offset)
      return std::unexpected(RelocationCode::BadOffset);
    else return bytes.subspan(rel.offset, *size);
  }

  // Values follow the relocation table of the RISC-V ELF psABI.
  Result apply_riscv(std::span<u8> field, const ResolvedRelocation &rel) const {
    using enum RV;
    const auto type = static_cast<RV>(rel.type);
    const u64 sa = absolute(rel);
    const auto relative = pc_relative(rel);
    const auto check_high = [this](i64 value) -> Result {
      // In rv32, lui/auipc can reach every address.
      if (width == pepp::bts::ElfBits::b64 && bits::signed_bitcount((value + 0x800) >> 12) > 20)
        return RelocationCode::Truncated;
      return std::nullopt;
    };

    switch (type) {
    case R_RISCV_32: [[fallthrough]];
    case R_RISCV_64: return write_absolute(field, order, sa);
    case R_RISCV_BRANCH:
      if (const auto error = check_jump(relative, 13)) return error;
      patch<riscv::InstructionB>(field, [&](auto &insn) { insn.set_imm(static_cast<u32>(relative)); });
      return std::nullopt;
    case R_RISCV_JAL:
      if (const auto error = check_jump(relative, 21)) return error;
      patch<riscv::InstructionJ>(field, [&](auto &insn) { insn.set_jump_offset(static_cast<u32>(relative)); });
      return std::nullopt;
    case R_RISCV_HI20:
      if (const auto error = check_high(static_cast<i64>(sa))) return error;
      patch<riscv::InstructionU>(field, [&](auto &insn) { insn.imm = high20(static_cast<i64>(sa)); });
      return std::nullopt;
    case R_RISCV_LO12_I:
      patch<riscv::InstructionI>(field, [&](auto &insn) { insn.imm = static_cast<u32>(sa); });
      return std::nullopt;
    case R_RISCV_LO12_S:
      patch<riscv::InstructionS>(field, [&](auto &insn) { insn.set_imm(static_cast<u32>(sa)); });
      return std::nullopt;
    case R_RISCV_PCREL_HI20:
      if (const auto error = check_high(relative)) return error;
      patch<riscv::InstructionU>(field, [&](auto &insn) { insn.imm = high20(relative); });
      return std::nullopt;
    // A PCREL_LO12 with its R_RISCV_PCREL_HI20 is a PairedRelocation.
    // Handled inline by operator()(const PairedRelocation&)
    case R_RISCV_PCREL_LO12_I: [[fallthrough]];
    case R_RISCV_PCREL_LO12_S: return RelocationCode::Unpaired;
    case R_RISCV_ADD8: [[fallthrough]];
    case R_RISCV_ADD16: [[fallthrough]];
    case R_RISCV_ADD32: [[fallthrough]];
    case R_RISCV_ADD64: add_to(field, order, sa); return std::nullopt;
    case R_RISCV_SUB8: [[fallthrough]];
    case R_RISCV_SUB16: [[fallthrough]];
    case R_RISCV_SUB32: [[fallthrough]];
    case R_RISCV_SUB64: add_to(field, order, u64(0) - sa); return std::nullopt;
    case R_RISCV_32_PCREL:
      if (bits::signed_bitcount(relative) > 32) return RelocationCode::Truncated;
      bits::memcpy_endian(field, order, static_cast<u64>(relative));
      return std::nullopt;
    default: return RelocationCode::UnsupportedType;
    }
  }
};
} // namespace

std::optional<pepp::link::RelocationKey> pepp::link::paired_key(bts::ElfMachineType machine, u64 address,
                                                                const ResolvedRelocation &rel) {
  using enum RV;
  const auto type = static_cast<RV>(rel.type);
  if (machine != bts::ElfMachineType::EM_RISCV || !rel.symbol) return std::nullopt;
  else if (type != R_RISCV_PCREL_LO12_I && type != R_RISCV_PCREL_LO12_S) return std::nullopt;
  // S + A is the address of an auipc in this section, whose R_RISCV_PCREL_HI20 supplies the value.
  const u64 sa = absolute(rel);
  if (sa < address) return std::nullopt;
  return RelocationKey{sa - address, bits::to_underlying(R_RISCV_PCREL_HI20)};
}

std::optional<pepp::link::RelocationCode>
pepp::link::apply_relocation(bts::ElfMachineType machine, bts::ElfBits width, bts::ElfEndian endian, u64 address,
                             std::span<u8> bytes,
                             const std::variant<ResolvedRelocation, PairedRelocation> &relocation) {
  const auto order = endian == bts::ElfEndian::be ? bits::Order::BigEndian : bits::Order::LittleEndian;
  return std::visit(Apply{machine, width, order, address, bytes}, relocation);
}
