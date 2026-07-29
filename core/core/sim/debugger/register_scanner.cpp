#include "register_scanner.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/system.hpp"

namespace {
static const Operation rw(Operation::Type::Application, Operation::Kind::data);
}
void RegisterScan::expose(const Register &n) {
  auto id = next_id();
  _regs[id] = std::make_unique<Register>(n);
  _exposed[n.target].push_back(id);
}

bits::Order RegisterScan::read(const RegisterRef &ref, bits::span<u8> dest, Byteswap bswap) {
  using namespace bits;
  // Structured bindins do not play well with QtCreator's debugger, so unpack the fields manually so I can inspect them.
  auto p = resolve(ref);
  auto reg = p.first;
  auto field = p.second;

  if (!reg) throw std::runtime_error("Register not found");
  else if (auto dev = _sys->find_by_id(reg->target); !dev) throw std::runtime_error("Device not found");
  else if (auto target = dev->capability<Target>(); !target) throw std::runtime_error("Device is not a Target");
  else {
    if (!any(reg->access & Register::Access::Read)) throw std::runtime_error("Register is not readable");
    // Trim the destination to the size of the register, in case the caller provided a buffer larger than the width.
    // have to choose first/last bast on host endianness so that later copy/swaps will work (e.g., 0-padding is on the
    // right end).
    if constexpr (bits::host_is_le) target->read(reg->offset, dest.last(reg->byte_width), rw);
    else target->read(reg->offset, dest.first(reg->byte_width), rw);
    if (field) {
      if (!any(field->access & Register::Access::Read)) throw std::runtime_error("Field is not readable");
      // Must convert to host-order so that bitmasking operations work
      auto copy = bits::memcpy_endian<u64>(dest, reg->order);
      // Shift the field down so least-significant bit is at [0], and mask out bits beyond width
      copy = (copy >> field->bit_offset) & ((1ULL << field->bit_width) - 1);
      // Copy back into dest, converting back to the register's order.
      bits::memcpy_endian(dest, reg->order, copy);
    }
    const bool should_swap =
        (bswap == Byteswap::Always) || (bswap == Byteswap::IfHostMismatch && reg->order != bits::hostOrder());
    if (should_swap) bits::bytereverse(dest);
    return reg->order;
  }
}

std::pair<RegisterScan::Register *, RegisterScan::Register::Field *> RegisterScan::resolve(RegisterRef r) {
  auto [reg, field] = std::as_const(*this).resolve(r);
  return {const_cast<Register *>(reg), const_cast<Register::Field *>(field)};
}

std::pair<const RegisterScan::Register *, const RegisterScan::Register::Field *>
RegisterScan::resolve(RegisterRef r) const {
  if (auto it = _regs.find(r.reg); it == _regs.end()) return {nullptr, nullptr};
  else if (auto &reg = it->second; r.is_register()) return {reg.get(), nullptr};
  else if (auto field_idx = static_cast<u16>(r.field.value) - 1; field_idx >= reg->fields.size())
    return {nullptr, nullptr};
  else return {reg.get(), &reg->fields[field_idx]};
}

std::optional<RegisterScan::RegisterRef> RegisterScan::find(std::string_view name) {
  for (const auto &it : _regs) {
    const auto id = it.first;
    const auto &reg = it.second;
    if (reg->name == name) return RegisterRef{id, Register::Field::ID{0}};
    for (int it = 0; it < reg->fields.size(); ++it) {
      if (auto f = reg->fields[it]; f.name == name)
        return RegisterRef{id, Register::Field::ID{static_cast<u16>(it + 1)}};
    }
  }
  return std::nullopt;
}

u8 RegisterScan::bit_width(RegisterRef r) const {
  auto [reg, field] = resolve(r);
  if (!reg) {
    throw std::runtime_error("Register not found");
  } else if (!field) return reg->byte_width * 8;
  else return field->bit_width;
}
RegisterScan::Register::Access RegisterScan::access(RegisterRef r) const {
  auto [reg, field] = resolve(r);
  if (!reg) {
    throw std::runtime_error("Register not found");
  } else if (!field) return reg->access;
  else return field->access;
}

RegisterScan::Register::ID RegisterScan::next_id() { return next++; }
