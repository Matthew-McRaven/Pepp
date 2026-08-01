#include "register_scanner.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/system.hpp"

namespace {
static const Operation rw(Operation::Type::Application, Operation::Kind::data);
static const Operation rw_buf(Operation::Type::BufferInternal, Operation::Kind::data);
}
void RegisterScan::expose(const Register &n) {
  auto id = next_id();
  _regs[id] = std::make_unique<Register>(n);
  _exposed[n.target].push_back(id);
}

void RegisterScan::write(const RegisterRef &ref, bits::span<const u8> src, Byteswap bswap) {
  using namespace bits;
  auto [reg, field] = resolve(ref);
  return write(reg, field, src, bswap);
}

bits::Order RegisterScan::read(const RegisterRef &ref, bits::span<u8> dest, Byteswap bswap) {
  auto [reg, field] = resolve(ref);
  return read(reg, field, dest, bswap);
}

void RegisterScan::clear(const RegisterRef &r) {
  auto [reg, field] = this->resolve(r);
  static const u64 zero = 0;
  static const auto zspan = bits::span<const u8>{reinterpret_cast<const u8 *>(&zero), sizeof(zero)};
  if (!reg) throw std::runtime_error("Register not found");
  write(reg, field, zspan, Byteswap::Never, true);
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

bits::Order RegisterScan::read(Register *reg, Register::Field *field, bits::span<u8> dest, Byteswap bswap) {
  using namespace bits;
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

void RegisterScan::write(Register *reg, Register::Field *field, bits::span<const u8> src, Byteswap bswap, bool force) {
  using namespace bits;
  if (!reg) throw std::runtime_error("Register not found");
  else if (auto dev = _sys->find_by_id(reg->target); !dev) throw std::runtime_error("Device not found");
  else if (auto target = dev->capability<Target>(); !target) throw std::runtime_error("Device is not a Target");
  else {
    // `force` ignores reset value, useful in clear.
    if (!force && !any(reg->access & Register::Access::Write)) throw std::runtime_error("Register is not writable");
    const size_t width = std::min<size_t>(reg->byte_width, sizeof(u64));

    // Byteswap describes src here, mirroring how it describes the destination in read().
    Order src_order = reg->order;
    switch (bswap) {
    case Byteswap::Never: break;
    case Byteswap::Always: src_order = reg->order == Order::BigEndian ? Order::LittleEndian : Order::BigEndian; break;
    case Byteswap::IfHostMismatch: src_order = hostOrder(); break;
    }
    // Promote to a fixed-sized type to make byte-reversal operations easier to understand.
    // memcpy_endian truncates or 0-pads when src isn't the register's width.
    u64 value = memcpy_endian<u64>(src, src_order);

    if (field) {
      if (!force && !any(field->access & Register::Access::Write))
        throw std::runtime_error("Field is not writable");
      // Read-modify-write: the bits outside this field may belong to other fields siblings and must be unchanged.
      // Create a copy in whole, then mask out the field's previous bits before inserting the result.
      // Ensure data is in host order before masking & shifting.
      u64 whole = 0;
      auto cur = span<u8>{reinterpret_cast<u8 *>(&whole), width};
      // Avoid MMIO on this access if it is memory-mapped.
      target->read(reg->offset, cur, rw_buf);
      whole = memcpy_endian<u64>(cur, reg->order);
      const u64 mask = field->bit_width >= 64 ? ~0ULL : (1ULL << field->bit_width) - 1;
      value = (whole & ~(mask << field->bit_offset)) | ((value & mask) << field->bit_offset);
    }

    // Convert in place: memcpy_endian takes its integral source by value, so it has already copied `value` before it
    // writes, and using value's own storage as the destination cannot alias. Which bytes of the u64 these are does
    // not matter -- memcpy_endian defines dest's contents outright, so no host-order assumption leaks in.
    auto out = span<u8>{reinterpret_cast<u8 *>(&value), width};
    memcpy_endian(out, reg->order, value);
    target->write(reg->offset, out, force ? rw_buf : rw);
  }
}

RegisterScan::Register::Access RegisterScan::access(RegisterRef r) const {
  auto [reg, field] = resolve(r);
  if (!reg) {
    throw std::runtime_error("Register not found");
  } else if (!field) return reg->access;
  else return field->access;
}

RegisterScan::Register::ID RegisterScan::next_id() { return next++; }
