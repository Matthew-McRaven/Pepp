#pragma once
#include <algorithm>
#include <array>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"

// Corresponds to an AddressSpan in some Target.

// A class that acts a bit like the scanchain of a JTAG debugger, allowing named entries to be exposed
// Devices call expose(...) as part of scan_debug_hardware(...), registering locations which can be read and written by
// a debugger.
class RegisterScan {
public:
  struct Register {
    using ID = pepp::OpaqueHandle<struct RegisterID, u16>;
    enum class Access : u8 { None = 0, Read = 1 << 0, Write = 1 << 1 };
    static constexpr auto ReadWrite = (Access)((u8)Access::Read | (u8)Access::Write);
    struct Field {
      // Starts from 1, not 0! 0 is a reserved value.
      using ID = pepp::OpaqueHandle<struct FieldID, u16>;
      Access access = ReadWrite;
      u8 bit_offset;
      u8 bit_width;
      std::string name;
    };
    struct Reference {
      Register::ID reg;
      Register::Field::ID field;

      bool is_register() const { return !is_field(); }
      bool is_field() const { return field.value != 0; }
    };
    bits::Order order;
    u8 byte_width; // Width in BYTES.
    Access access = ReadWrite;
    Device::ID target;
    Address offset;
    std::string name;
    std::vector<Field> fields;
  };
  using RegisterRef = Register::Reference;

  RegisterScan(System *sys) : _sys(sys) {}
  void expose(const Register &n);
  // Copy
  enum class Byteswap {
    Always,        // Always perform a byteswap, even when host/guest match.
    Never,         // Never perform a byteswap, even when host/guest mismatch.
    IfHostMismatch // If the register's order does not match the host order, byteswap in dest before returning.
  };

  bits::Order read(const RegisterRef &n, bits::span<u8> dest, Byteswap bswap = Byteswap::Never);
  std::optional<RegisterRef> find(std::string_view name);
  // Helper which returns the value of a register as an integral type
  template <std::integral I> I read(const RegisterRef &n);

  std::pair<Register *, Register::Field *> resolve(RegisterRef r);
  std::pair<const Register *, const Register::Field *> resolve(RegisterRef r) const;
  Register::Access access(RegisterRef r) const;
  u8 bit_width(RegisterRef r) const;

private:
  Register::ID next_id();

  System *_sys;
  Register::ID next{static_cast<u16>(1)};
  std::unordered_map<Device::ID, std::list<Register::ID>> _exposed;
  // Store Registers in a unique_ptr to avoid invalidating pointers on re-hash.
  std::unordered_map<Register::ID, std::unique_ptr<Register>, pepp::handle_hash<Register::ID>> _regs;
};

template <std::integral I> I RegisterScan::read(const RegisterRef &n) {
  auto p = resolve(n);
  if (!p.first) throw std::runtime_error("Register not found");
  // Read into a fixed-size buffer so that we know where the bytes will land before we convert to host order.
  // this avoids a posibility where we read the wrong "part" of a register and therefore return 0.
  const size_t width = std::min<size_t>(p.first->byte_width, sizeof(u64));
  std::array<u8, sizeof(u64)> buf{};
  const auto order = read(n, bits::span<u8>{buf.data(), width}, Byteswap::Never);
  return (I)bits::memcpy_endian<u64>(bits::span<const u8>{buf.data(), width}, order);
}

consteval void is_bitflags(RegisterScan::Register::Access);
consteval void allow_opaque_handle_increment(RegisterScan::Register::ID);
