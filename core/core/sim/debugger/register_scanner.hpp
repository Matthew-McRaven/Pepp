#pragma once
#include <algorithm>
#include <array>
#include <initializer_list>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"

// opt-in increment found by ADL. We need to ensure that this tag is visible before first use as a template argument,
// which is inside of the body of RegisterScan.
struct RegisterID;
consteval void allow_opaque_handle_increment(pepp::OpaqueHandle<RegisterID, u16>);

// Corresponds to an AddressSpan in some Target.

// A class that acts a bit like the scanchain of a JTAG debugger, allowing named entries to be exposed
// Devices call expose(...) as part of initialize(...), registering locations which can be read and written by
// a debugger.
class RegisterScan {
public:
  struct Register {
    using ID = pepp::OpaqueHandle<struct RegisterID, u16>;
    // Operations allowed on a register or field.
    enum class Access : u8 { None = 0, Read = 1 << 0, Write = 1 << 1 };
    // Describe reset behavior of this register and how it is updated over the duration of a simulation.
    enum class Kind : u8 {
      // Machine state that has a power-on value which must be restored on reset and changes (unpredictably) over time.
      // The Pep/10 accumulator or program counter are examples of such registers.
      State,
      // A monotonic accumulator that is additive across devices. For example, summing rd_bytes over every RAM is
      // meaningful. Its reset value must be 0, and it should be resetable at arbitrary times
      Count,
      // An instantaneous, non-additive measurement, in the OpenTelemetry sense of the word. The owning device is
      // responsible for updating it appropriately on step-forward and -backwards, potentially using the trace system.
      Gauge,
    };
    // Who can observe the register. A guest-readable cycle counter would be Count + Architectural, while an instruction
    // tally the simulator keeps for itself is Count + Internal.
    enum class Visibility : u8 {
      Architectural,      // Part of the ISA of the system.
      Microarchitectural, // A microarchitecture detail, but still part of the modeled implementation.
      // No instructions in the ISA directly touch the registers, so a guest program cannot read nor modify it directly.
      // That being said, Level::Guest reads/writes may still be permitted because a debugger is making modifications on
      // the user's behalf.
      Internal,
    };

    static constexpr auto ReadWrite =
        static_cast<Access>(static_cast<u8>(Access::Read) | static_cast<u8>(Access::Write));
    struct Field {
      // Starts from 1, not 0! 0 is a reserved value.
      using ID = pepp::OpaqueHandle<struct FieldID, u16>;
      Access guest_access = ReadWrite;
      Access host_access = ReadWrite;
      u8 bit_offset = 0;
      u8 bit_width = 0;
      std::string name;
    };
    struct Reference {
      Register::ID reg;
      Register::Field::ID field;

      bool is_register() const { return !is_field(); }
      bool is_field() const { return field.value != 0; }
    };
    u8 byte_width; // Width in BYTES.
    // Separate guest access from host access. A debugger may need access to a register/counter (e.g., call_depth) that
    // should either be invisible to the guest or read-only.
    Access guest_access = ReadWrite;
    Access host_access = ReadWrite;
    // If true, the register's value will be correctly updated on step-backwards/undo, either by using an explict trace
    // or recomputing the value as necessary. If false, the register's value won't be restored on step-backwards and may
    // be incorrect for the remainder of the run. Micro/architecturally significant registers should be restored,
    // otherwise step-back will not be accurate. Internal registers used to derive execution statistics can
    // choose not to be restored, since they do not impact the correctness of the run.
    bool restore_on_step_back = true;
    Kind kind = Kind::State;
    Visibility visibility = Visibility::Architectural;
    Device::ID target;
    bits::Order order;
    std::string name;
    std::vector<Field> fields;
    // if Address, then fetch the target from a system, cast to Target* and call the appropirate method.
    // Otherwise read/write the pointer directly.
    using StorageLocation = std::variant<std::monostate, Address, i8 *, u8 *, i16 *, u16 *, i32 *, u32 *, i64 *, u64 *>;
    StorageLocation loc = std::monostate{};
  };
  using RegisterRef = Register::Reference;
  using Access = Register::Access;

  RegisterScan(System *sys) : _sys(sys) {}
  RegisterScan::Register::Reference expose(const Register &n);

  // Where does the access originate from? Both techincally come from within this process, but distinguishing simulated
  // traffic from the simulators' infrastructure's traffic is important. If the access is made on the machines behalf
  // (e.g., a debugger poking a register to display it), the access should be Guest. Trying to reset a VM to a
  // clean/default state or replaying a trace would be Host. The default is Guest, which is the least-permissive.
  enum class Level : u8 {
    Guest, // The system/device tree under test, and anything reaching into it on a user's behalf.
    Host,  // The simulator's own infrastructure: replaying a trace, resetting a device, restoring a checkpoint.
  };

  enum class Byteswap {
    Always,        // Always perform a byteswap, even when host/guest match.
    Never,         // Never perform a byteswap, even when host/guest mismatch.
    IfHostMismatch // If the register's order does not match the host order, byteswap in dest before returning.
  };

  void write(const RegisterRef &n, bits::span<const u8> src, Byteswap bswap = Byteswap::Never,
             Level level = Level::Guest);
  bits::Order read(const RegisterRef &n, bits::span<u8> dest, Byteswap bswap = Byteswap::Never,
                   Level level = Level::Guest);

  // If id is non-0, only match against registers which share the same target ID. If ID==0, match against all registers.
  std::optional<RegisterRef> find(std::string_view name, Device::ID scope = Device::ID{0});
  // Helper which returns the value of a register as an integral type
  template <std::integral I> I read(const RegisterRef &n, Level level = Level::Guest);
  // Helper which writes an integral value to a register.
  template <std::integral I> void write(const RegisterRef &n, I value, Level level = Level::Guest);
  // A reset rather than a write, so it goes in at Level::Host: a register the guest may not write still resets.
  void clear(const RegisterRef &n);
  // Reset every exposed register of the given kind. Host-unwritable registers are skipped.
  std::size_t reset(std::initializer_list<Register::Kind> kinds);

  std::pair<Register *, Register::Field *> resolve(RegisterRef r);
  std::pair<const Register *, const Register::Field *> resolve(RegisterRef r) const;
  Register::Access access(RegisterRef r, Level level = Level::Guest) const;
  u8 bit_width(RegisterRef r) const;

private:
  bits::Order read(Register *, Register::Field *, bits::span<u8> dest, Byteswap bswap, Level level);
  // Fetch a register's bytes out of whatever backs it without checking for guest-read access permission.
  void load(Register *reg, bits::span<u8> out, Operation access);
  void write(Register *, Register::Field *, bits::span<const u8> src, Byteswap bswap, Level level);
  // What this register/field grants at `level`.
  static Access granted(const Register &reg, Level level);
  static Access granted(const Register::Field &field, Level level);
  Register::ID next_id();

  System *_sys;
  Register::ID next{static_cast<u16>(1)};
  std::unordered_map<Device::ID, std::list<Register::ID>> _exposed;
  // Store Registers in a unique_ptr to avoid invalidating pointers on re-hash.
  std::unordered_map<Register::ID, std::unique_ptr<Register>, pepp::handle_hash<Register::ID>> _regs;
};

template <std::integral I> I RegisterScan::read(const RegisterRef &n, Level level) {
  auto p = resolve(n);
  if (!p.first) throw std::runtime_error("Register not found");
  // Read into a fixed-size buffer so that we know where the bytes will land before we convert to host order.
  // this avoids a posibility where we read the wrong "part" of a register and therefore return 0.
  const size_t width = std::min<size_t>(p.first->byte_width, sizeof(u64));
  std::array<u8, sizeof(u64)> buf{};
  const auto order = read(p.first, p.second, bits::span<u8>{buf.data(), width}, Byteswap::Never, level);
  return (I)bits::memcpy_endian<u64>(bits::span<const u8>{buf.data(), width}, order);
}

template <std::integral I> void RegisterScan::write(const RegisterRef &n, I value, Level level) {
  auto p = resolve(n);
  if (!p.first) throw std::runtime_error("Register not found");
  // Serialize into the register's own order up front, so the write path has nothing left to convert. This mirrors
  // read<I>, which pulls the bytes out raw with Byteswap::Never and converts to host order here.
  const size_t width = std::min<size_t>(p.first->byte_width, sizeof(u64));
  u64 raw = 0;
  auto buf = bits::span<u8>{reinterpret_cast<u8 *>(&raw), width};
  // Cast sign-extends according to the rules of I.
  bits::memcpy_endian(buf, p.first->order, (u64)value);
  write(p.first, p.second, buf, Byteswap::Never, level);
}

consteval void is_bitflags(RegisterScan::Register::Access);
