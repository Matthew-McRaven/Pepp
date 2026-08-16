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
    for (int inner = 0; inner < reg->fields.size(); ++inner) {
      if (auto f = reg->fields[inner]; f.name == name)
        return RegisterRef{id, Register::Field::ID{static_cast<u16>(inner + 1)}};
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

struct ReadVisit {
  RegisterScan::Register *reg;
  bits::span<u8> out;
  void operator()(std::monostate) { throw std::runtime_error("Register has no storage location"); }
  void operator()(Address addr) { throw std::runtime_error("Device is not a target"); }
  template <typename T> void operator()(const T *ptr) {
    if (sizeof(T) != out.size()) throw std::runtime_error("Register width mismatch");
    std::memcpy(out.data(), ptr, sizeof(T));
  }
};

bits::Order RegisterScan::read(Register *reg, Register::Field *field, bits::span<u8> dest, Byteswap bswap) {
  return read(reg, field, dest, bswap, rw);
}
bits::Order RegisterScan::read(Register *reg, Register::Field *field, bits::span<u8> dest, Byteswap bswap,
                               Operation access) {
  using namespace bits;
  if (!reg) throw std::runtime_error("Register not found");
  else if (std::holds_alternative<std::monostate>(reg->loc))
    throw std::runtime_error("Register has no storage location");

  // Trim the destination to the size of the register, in case the caller provided a buffer larger than the width.
  // ave to choose first/last based on host endianness so that later copy/swaps will work (e.g., 0-padding is on the
  // right end). Only work on this trimmed view.
  if (dest.size() < reg->byte_width) throw std::runtime_error("Destination is narrower than the register");
  const auto out = bits::host_is_le ? dest.last(reg->byte_width) : dest.first(reg->byte_width);

  if (!std::holds_alternative<Address>(reg->loc)) {
    std::visit(ReadVisit{.reg = reg, .out = out}, reg->loc);
  } else if (auto dev = _sys->find_by_id(reg->target); !dev) throw std::runtime_error("Device not found");
  else if (auto target = dev->capability<Target>(); !target) throw std::runtime_error("Device is not a Target");
  else {
    if (!any(reg->access & Register::Access::Read)) throw std::runtime_error("Register is not readable");
    target->read(std::get<Address>(reg->loc), out, access);
  }

  if (field) {
    if (!any(field->access & Register::Access::Read)) throw std::runtime_error("Field is not readable");
    // Must convert to host-order so that bitmasking operations work
    auto copy = bits::memcpy_endian<u64>(out, reg->order);
    // Shift the field down so least-significant bit is at [0], and mask out bits beyond width
    copy = (copy >> field->bit_offset) & ((1ULL << field->bit_width) - 1);
    // Copy back into dest, converting back to the register's order.
    bits::memcpy_endian(out, reg->order, copy);
  }
  const bool should_swap =
      (bswap == Byteswap::Always) || (bswap == Byteswap::IfHostMismatch && reg->order != bits::hostOrder());
  if (should_swap) bits::bytereverse(out);
  return reg->order;
}

struct WriteVisit {
  RegisterScan::Register *reg;
  bits::span<const u8> src;
  void operator()(std::monostate) { throw std::runtime_error("Register has no storage location"); }
  void operator()(Address addr) { throw std::runtime_error("Device is not a target"); }
  template <typename T> void operator()(T *ptr) {
    if (sizeof(T) != src.size()) throw std::runtime_error("Register width mismatch");
    std::memcpy(ptr, src.data(), sizeof(T));
  }
};

void RegisterScan::write(Register *reg, Register::Field *field, bits::span<const u8> src, Byteswap bswap, bool force) {
  using namespace bits;
  if (!reg) throw std::runtime_error("Register not found");
  else if (std::holds_alternative<std::monostate>(reg->loc))
    throw std::runtime_error("Register has no storage location");

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
    if (!force && !any(field->access & Register::Access::Write)) throw std::runtime_error("Field is not writable");
    // Read-modify-write: the bits outside this field may belong to other fields siblings and must be unchanged.
    // Create a copy in whole, then mask out the field's previous bits before inserting the result.
    // Ensure data is in host order before masking & shifting.
    u64 whole = 0;
    auto cur = span<u8>{reinterpret_cast<u8 *>(&whole), width};
    // Avoid MMIO on this access if it is memory-mapped.
    read(reg, nullptr, cur, Byteswap::Never, rw_buf);
    whole = memcpy_endian<u64>(cur, reg->order);
    const u64 mask = field->bit_width >= 64 ? ~0ULL : (1ULL << field->bit_width) - 1;
    value = (whole & ~(mask << field->bit_offset)) | ((value & mask) << field->bit_offset);
  }

  // Convert in place: memcpy_endian takes its integral source by value, so it has already copied `value` before it
  // writes, and using value's own storage as the destination cannot alias. Which bytes of the u64 these are does
  // not matter -- memcpy_endian defines dest's contents outright, so no host-order assumption leaks in.
  auto out = span<u8>{reinterpret_cast<u8 *>(&value), width};
  memcpy_endian(out, reg->order, value);

  if (auto dev = _sys->find_by_id(reg->target); !dev) throw std::runtime_error("Device not found");
  else if (!std::holds_alternative<Address>(reg->loc)) {
    std::visit(WriteVisit{.reg = reg, .src = out}, reg->loc);
  } else if (auto target = dev->capability<Target>(); !target) throw std::runtime_error("Device is not a Target");
  else {
    if (!force && !any(reg->access & Register::Access::Write)) throw std::runtime_error("Register is not writable");
    target->write(std::get<Address>(reg->loc), out, force ? rw_buf : rw);
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
