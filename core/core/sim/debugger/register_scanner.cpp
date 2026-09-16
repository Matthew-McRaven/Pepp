#include "register_scanner.hpp"
#include <stdexcept>
#include <unordered_set>
#include "core/ds/hash/fnv.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/mask.hpp"
#include "core/sim/system.hpp"

namespace {
static const Operation rw(Operation::Type::Application, Operation::Kind::data);
static const Operation rw_buf(Operation::Type::BufferInternal, Operation::Kind::data);

// A pointer-backed register must declare the width of the storage it points at, because that is what the access
// visitors compare against on every read and write. Checked once at expose() so a mistake fails where the register is
// declared, rather than as an exception out of the middle of some later read. An Address has no size to check.
struct WidthCheck {
  const RegisterScan::Register &reg;
  void operator()(std::monostate) const {}
  void operator()(Address) const {}
  template <typename T> void operator()(const T *) const {
    if (sizeof(T) != reg.byte_width)
      throw std::logic_error("Register " + reg.name + " declares " + std::to_string(reg.byte_width) +
                             " bytes but points at storage of " + std::to_string(sizeof(T)));
  }
};
} // namespace
RegisterScan::Register::Reference RegisterScan::expose(const Register &n) {
  std::visit(WidthCheck{n}, n.loc);
  auto id = next_id();
  _regs[id] = std::make_unique<Register>(n);
  _exposed[n.target].push_back(id);
  return Register::Reference{.reg = id};
}

void RegisterScan::write(const RegisterRef &ref, bits::span<const u8> src, Byteswap bswap, Level level) {
  using namespace bits;
  auto [reg, field] = resolve(ref);
  return write(reg, field, src, bswap, level);
}

bits::Order RegisterScan::read(const RegisterRef &ref, bits::span<u8> dest, Byteswap bswap, Level level) {
  auto [reg, field] = resolve(ref);
  return read(reg, field, dest, bswap, level);
}

std::optional<RegisterScan::RegisterRef> RegisterScan::find(std::string_view name) {
  const auto colon = name.rfind(':');
  // No scope, so search all registers. This retains the old behavior of find with a defaulted Device::ID{0}.
  if (colon == std::string_view::npos) return find(name, Device::ID{0});
  const auto device = name.substr(0, colon), reg = name.substr(colon + 1);
  if (device.empty()) return find(reg, Device::ID{0});
  else if (_sys == nullptr) return std::nullopt;
  else if (auto dev = _sys->find_absolute(device); dev == nullptr) return std::nullopt;
  else return find(reg, dev->id());
}

std::optional<RegisterScan::RegisterRef> RegisterScan::find(std::string_view name, Device::ID scope) {
  struct Match {
    // If nullopt, either 0 or 2+ matches were found. If set, it is the unique match.
    std::optional<RegisterRef> ref = std::nullopt;
    // If true, multiple registers matched the name. Ref is meaningless and set to nullopt.
    bool ambiguous = false;
  };
  // Accept is a predicate acting on device IDs. If it returns true, then you should compare against that device's
  // registers. If false, continue to next registers. Allows our three kinds of searches to share the same inner loop.
  auto search = [&](auto &&accept) {
    Match match;
    // TODO: inefficient linear search
    for (const auto &it : _regs) {
      const auto id = it.first;
      const auto &reg = it.second;
      if (!accept(reg->target)) continue;
      // Try to match on the register's full name
      else if (reg->name == name) {
        if (match.ref) return Match{std::nullopt, true};
        else match.ref = RegisterRef{id, Register::Field::ID{0}};
      }
      // Otherwise try to match on the register's fields.
      for (int inner = 0; inner < reg->fields.size(); ++inner) {
        if (auto f = reg->fields[inner]; f.name == name) {
          if (match.ref) return Match{std::nullopt, true};
          else match.ref = RegisterRef{id, Register::Field::ID{static_cast<u16>(inner + 1)}};
        }
      }
    }
    return match;
  };

  // ID==0 is the parent of all devices, so search everything without building an explicit set.
  if (scope.value == 0) return search([](Device::ID) { return true; }).ref;
  auto node = _sys == nullptr ? nullptr : _sys->find_tree_by_id(scope);
  if (node == nullptr) return std::nullopt;

  // Prefer a register in the scope itself, in case both parent and child define a name.
  if (auto own = search([scope](Device::ID target) { return target == scope; }); own.ref || own.ambiguous)
    return own.ref;

  // The set of all devices below scope. Collected once rather than walking the tree for every register.
  std::unordered_set<Device::ID, pepp::handle_hash<Device::ID>> in_scope;
  for (const auto dev : *node)
    if (dev->id() != scope) in_scope.insert(dev->id());
  // Otherwise try to find a match amongst the device's children.
  return search([&](Device::ID target) { return in_scope.contains(target); }).ref;
}

void RegisterScan::clear(const RegisterRef &r) {
  auto [reg, field] = this->resolve(r);
  static const u64 zero = 0;
  static const auto zspan = bits::span<const u8>{reinterpret_cast<const u8 *>(&zero), sizeof(zero)};
  if (!reg) throw std::runtime_error("Register not found");
  // A reset brings the register back to its default value, simulating a power-cycle of the device. This should allow a
  // RO register (from the guest side) to be written.
  write(reg, field, zspan, Byteswap::Never, Level::Host);
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

std::size_t RegisterScan::reset(std::initializer_list<Register::Kind> kinds) {
  using namespace bits;
  std::size_t count = 0;
  for (const auto &[id, reg] : _regs) {
    if (std::find(kinds.begin(), kinds.end(), reg->kind) == kinds.end()) continue;
    // Reject a host write is how a device says it maintains this itself, so a reset has nothing to do here.
    else if (!any(reg->host_access & Register::Access::Write)) continue;
    clear(RegisterRef{id, Register::Field::ID{0}});
    ++count;
  }
  return count;
}

// Initialize the hash with the correct initial value for our FNV-a1 hash algorithm.
RegisterScan::Selection::Selection(RegisterScan *scan) : _scan(scan), _hash(pepp::fnv_1a_basis()) {}
RegisterScan::Selection::Selection() : Selection(nullptr) {}

bool RegisterScan::Selection::add(RegisterRef ref) {
  using namespace bits;
  // A field would sample fine, but its width is bits where delta_to() subtracts at a byte width -- refuse rather
  // than quietly widening it to the register it lives in.
  if (_scan == nullptr || ref.is_field()) return false;
  const auto [reg, field] = _scan->resolve(ref);
  // Reduce sanity-checking work in sample, discarding unreadable registers, those without storage, and those larger
  // than a sample slot.
  if (reg == nullptr) return false;
  else if (std::holds_alternative<std::monostate>(reg->loc)) return false;
  else if (!any(_scan->access(ref, Level::Host) & Register::Access::Read)) return false;
  else if (reg->byte_width > sizeof(u64)) return false;
  _regs.push_back(ref.reg);
  _widths.push_back(reg->byte_width);
  _hash = pepp::fnv_1a(ref.reg.value, _hash);
  return true;
}

std::optional<std::size_t> RegisterScan::Selection::index_of(RegisterRef ref) const {
  // TODO: if we re-sorted registers on insert this could be lg(n) rather than n
  if (ref.is_field()) return std::nullopt;
  else if (const auto it = std::find(_regs.begin(), _regs.end(), ref.reg); it == _regs.end()) return std::nullopt;
  else return static_cast<std::size_t>(std::distance(_regs.begin(), it));
}

std::optional<u64> RegisterScan::Selection::value_of(const Sample &s, RegisterRef ref) const {
  // Avoid accessing a sample which does not belong to this selection.
  if (s.hash() != _hash) throw std::runtime_error("Sample was not taken from this Selection");
  else if (const auto at = index_of(ref); !at) return std::nullopt;
  else return s.values()[*at];
}

std::optional<u64> RegisterScan::Selection::delta_of(const Sample &before, const Sample &after, RegisterRef ref) const {
  if (before.hash() != _hash || after.hash() != _hash) throw std::runtime_error("Sample not taken from this Selection");
  else if (const auto at = index_of(ref); !at) return std::nullopt;
  // Same as Sample::delta_to(), but for a single register rather than the whole sample to reduce runtime cost.
  else return (after.values()[*at] - before.values()[*at]) & bits::mask(_widths[*at]);
}

RegisterScan::Sample RegisterScan::Selection::sample() const {
  Sample out;
  out._hash = _hash;
  out._widths = _widths;
  out._values.reserve(_regs.size());
  for (const auto id : _regs)
    out._values.push_back(_scan->read<u64>(RegisterRef{id, Register::Field::ID{0}}, Level::Host));
  return out;
}

std::vector<u64> RegisterScan::Sample::delta_to(const Sample &after) const {
  if (_hash != after._hash) throw std::runtime_error("Samples come from different Selections");
  else if (_values.size() != after._values.size()) throw std::runtime_error("Sample sizes disagree");
  std::vector<u64> out;
  out.reserve(_values.size());
  for (std::size_t i = 0; i < _values.size(); ++i)
    out.push_back((after._values[i] - _values[i]) & bits::mask(_widths[i]));
  return out;
}

u8 RegisterScan::bit_width(RegisterRef r) const {
  auto [reg, field] = resolve(r);
  if (!reg) throw std::runtime_error("Register not found");
  else if (!field) return reg->byte_width * 8;
  else return field->bit_width;
}

struct ReadVisit {
  RegisterScan::Register *reg;
  bits::span<u8> out;
  void operator()(std::monostate) { throw std::runtime_error("Register has no storage location"); }
  void operator()(Address addr) { throw std::runtime_error("Device is not a target"); }
  template <typename T> void operator()(const T *ptr) {
    if (sizeof(T) != out.size()) throw std::runtime_error("Register width mismatch");
    else if (ptr == nullptr) throw std::runtime_error("Register pointer is null");
    std::memcpy(out.data(), ptr, sizeof(T));
  }
};

RegisterScan::Access RegisterScan::granted(const Register &reg, Level level) {
  return level == Level::Host ? reg.host_access : reg.guest_access;
}

RegisterScan::Access RegisterScan::granted(const Register::Field &field, Level level) {
  return level == Level::Host ? field.host_access : field.guest_access;
}

void RegisterScan::load(Register *reg, bits::span<u8> out, Operation access) {
  // The device is only consulted for Address-backed storage; a pointer needs nothing from the system.
  if (!std::holds_alternative<Address>(reg->loc)) {
    std::visit(ReadVisit{.reg = reg, .out = out}, reg->loc);
  } else if (auto dev = _sys->find_by_id(reg->target); !dev) throw std::runtime_error("Device not found");
  else if (auto target = dev->capability<Target>(); !target) throw std::runtime_error("Device is not a Target");
  else target->read(std::get<Address>(reg->loc), out, access);
}

bits::Order RegisterScan::read(Register *reg, Register::Field *field, bits::span<u8> dest, Byteswap bswap,
                               Level level) {
  using namespace bits;
  if (!reg) throw std::runtime_error("Register not found");
  else if (std::holds_alternative<std::monostate>(reg->loc))
    throw std::runtime_error("Register has no storage location");

  // Trim the destination to the size of the register, in case the caller provided a buffer larger than the width.
  // ave to choose first/last based on host endianness so that later copy/swaps will work (e.g., 0-padding is on the
  // right end). Only work on this trimmed view.
  if (dest.size() < reg->byte_width) throw std::runtime_error("Destination is narrower than the register");
  const auto out = bits::host_is_le ? dest.last(reg->byte_width) : dest.first(reg->byte_width);

  // Access control is first enforced at a per-register level before accessing.
  if (!any(granted(*reg, level) & Register::Access::Read)) throw std::runtime_error("Register is not readable");

  load(reg, out, level == Level::Guest ? rw : rw_buf);

  if (field) {
    if (!any(granted(*field, level) & Register::Access::Read)) throw std::runtime_error("Field is not readable");
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
    else if (ptr == nullptr) throw std::runtime_error("Register pointer is null");
    std::memcpy(ptr, src.data(), sizeof(T));
  }
};

void RegisterScan::write(Register *reg, Register::Field *field, bits::span<const u8> src, Byteswap bswap, Level level) {
  using namespace bits;
  if (!reg) throw std::runtime_error("Register not found");
  else if (std::holds_alternative<std::monostate>(reg->loc))
    throw std::runtime_error("Register has no storage location");
  // Ibid read(): the declaration binds whatever the storage is, and which half of it applies depends on the level.
  else if (!any(granted(*reg, level) & Register::Access::Write)) throw std::runtime_error("Register is not writable");

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
    if (!any(granted(*field, level) & Register::Access::Write)) throw std::runtime_error("Field is not writable");
    // Read-modify-write. Bits outside this field belong to other fields siblings and must be unchanged.
    // Create a copy which is masked and shifted to the individual field.
    // Ensure data is in host order before masking & shifting.
    u64 whole = 0;
    auto cur = span<u8>{reinterpret_cast<u8 *>(&whole), width};
    // Avoid mmio and do not require guest-read permission
    load(reg, cur, rw_buf);
    whole = memcpy_endian<u64>(cur, reg->order);
    const u64 mask = field->bit_width >= 64 ? ~0ULL : (1ULL << field->bit_width) - 1;
    value = (whole & ~(mask << field->bit_offset)) | ((value & mask) << field->bit_offset);
  }

  // Ensure working copy has been transformed t the register's order.
  auto out = span<u8>{reinterpret_cast<u8 *>(&value), width};
  memcpy_endian(out, reg->order, value);

  if (!std::holds_alternative<Address>(reg->loc)) {
    std::visit(WriteVisit{.reg = reg, .src = out}, reg->loc);
  } else if (auto dev = _sys->find_by_id(reg->target); !dev) throw std::runtime_error("Device not found");
  else if (auto target = dev->capability<Target>(); !target) throw std::runtime_error("Device is not a Target");
  // Only a guest write is an Application access; the host's own writes must not trace themselves.
  else target->write(std::get<Address>(reg->loc), out, level == Level::Guest ? rw : rw_buf);
}

RegisterScan::Register::Access RegisterScan::access(RegisterRef r, Level level) const {
  auto [reg, field] = resolve(r);
  if (!reg) {
    throw std::runtime_error("Register not found");
  } else if (!field) return granted(*reg, level);
  else return granted(*field, level);
}

RegisterScan::Register::ID RegisterScan::next_id() { return next++; }
