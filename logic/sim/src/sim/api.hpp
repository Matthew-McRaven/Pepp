#pragma once
#include <QtCore>
#include <type_traits>
namespace sim::api {

namespace Device {
using ID = quint16; // Only use 9 bits (max of 512)!
struct Descriptor {
  Device::ID id;
  void *compatible;
  QString baseName, fullName;
};
} // namespace Device

namespace Packet {
// clang-format off
/*
 * Flags allow a device to cast a memory location to the correct kind of Packet
 * by encoding type information.
 * scope:
 *   0: The meaning of the flags do not depend on the ID field of the Packet
 *   1: One must delegate to the device that created the packet to
 *      return type information
 * dyn:
 *   0: The containing packet's payload contains no fields who point to
 *      heap-allocated object. No destructor is necessary
 *   1: The containg packet's payload contains at least one field who points to
 *      a heap-allocated  object. If scope==0, then the Buffer is responsible
 *      for finding and calling a suitable destructor for the packet. If
 *      scope==1, the Buffer may delegate to the device which created the
 *      packet.
 * kind:
 *   When scope==0, all packets with the same kind bits must have identical
 *   storage layouts. When scope==1, only packets belonging to the same device
 *   must have the same storage layout.
 *
 * Special values:
 *   0b0'0'000'000 indicates and empty Packet.
 *   0bx'x'111'111 indicates an uninitialized type.
 */
 //clang-format on
struct Flags {
  quint8 scope : 1; // 0=global, 1=specific to device;
  quint8 dyn : 1;   // 1=some data allocated with malloc/new. Must be destroyed.
  quint8 kind : 6;
  inline operator quint16() const {
    union {
      Flags flags;
      quint16 bits;
    } type;
    type.flags = *this;
    return type.bits;
  }
};
static_assert(sizeof(Flags) == sizeof(quint8));



#pragma pack(push, 1)
struct EmptyPacket {
  union {
    quint8 size = 0;
    quint8 type;
  } const field;
};

/*
 * Assume there is a Packet<T>* ptr;
 * if *(quint8*)ptr ==0, then Packet<T> is really an EmptyPacket.
 *
 * That is, in the empty case, no fields are present, and the size and type
 * fields collapse to 0. While technically a EmptyPacket is size 1, this rule
 * gives us the nice property that when we start at a 0 in a array of Packets,
 * we can safely skip to the next non-zero byte.
 *
 * However, this means that type != 0 for all real Packets, otherwise the packet
 * will be misinterpreted.
 */

template <typename Payload> struct Packet {
  quint8 length = sizeof(Packet);
  Payload payload = {};
  Device::ID device = 0;
  union {
    Flags flags;
    quint8 bits =
        0b0'0'111'111; // Mark the type as unitialized rather that empty (0).
  } type;

  Packet() {}
  Packet(Device::ID id, Flags flags): device(device), payload(), type(flags){}
  // Must be declared inline, otherwise fails to compile.
  template <typename Bytes>
  Packet(Device::ID device, Bytes bytes, Flags flags)
      : device(device), payload(), type(flags) {
    void *dst, *src;
    if constexpr (std::is_pointer_v<std::decay_t<Payload>>)
      dst = this->payload;
    else
      dst = &this->payload;
    if constexpr (std::is_pointer_v<std::decay_t<Bytes>>)
      src = bytes;
    else
      src = &bytes;
    // Should use bits::memcpy, but don't want to make bits library a
    // requirement for API.
    memcpy(dst, src, qMin(sizeof(Bytes), sizeof(Payload)));
  };
};
#pragma pack(pop)

struct Registry {
  virtual ~Registry() = default;
  typedef void (*packet_dtor)(void *);
  virtual void registerDTOR(Flags flags, packet_dtor dtor) = 0;
  virtual packet_dtor getDTOR(Flags flags) = 0; // nullptr indicates no DTOR.
};
} // namespace Packet

namespace Trace {

// Forward declare, will be needed inside analyzer.
struct Buffer;

struct Analyzer {
  virtual ~Analyzer() = 0;
};
Analyzer::~Analyzer() = default;

struct Buffer {
  using AnalyzerHookID = quint32;
  enum class Status : quint8 {
    Success = 0,        // Object is now pending.
    OverflowAndSuccess, // Operation succeeded, but the next operation might
                        // not. Sync the buffer. Object IS pending.
    OverflowAndRetry,   // Operation did not succeed. Retry current step after
                        // sync'ing buffer. Object is NOT pending.
  };
  virtual ~Buffer() = default;
  // To avoid double-buffering, request that the buffer provide a sufficient
  // number of bytes. Callers can then use placement new to construct their
  // Packet in-place. If traceDest is nullptr, do not attempt to perform
  // placement.
  virtual Status request(quint8 length, void **traceDest) = 0;
  virtual void trace(quint16 deviceID, bool enabled = true) = 0;
  virtual void setPacketRegistry(api::Packet::Registry *registry) = 0;

  [[nodiscard]] virtual AnalyzerHookID registerAnalyzer(Analyzer *analyzer) = 0;
  [[nodiscard]] virtual Analyzer *unregisterAnalyzer(AnalyzerHookID id) = 0;
};

struct Producer {
  virtual ~Producer() = default;
  virtual void setTraceBuffer(Buffer *tb) = 0;
  virtual bool applyTrace(void *trace) = 0;   // trace is a Packet struct.
  virtual bool unapplyTrace(void *trace) = 0; // trace is a Packet struct.
};
} // namespace Trace

namespace Tick {
using Type = quint32; // System will crash at 2^32 ticks.
enum class Error : quint8 {
  Success = 0, // Scheduler should re-schedule this device at the next available
               // clock interval.
  NoMMInput,  // Scheduler should suspend execution of all devices until more MM
              // input is provided.
  Terminate,  // Scheduler should terminate execution of all devices, as the
              // device has entered an invalid state.
  Breakpoint, // Scheduler should suspend execution of all devices until
              // "resume" or "step" is hit.
};

struct Result {
  bool pause; // After this tick, should control be returned to execution
              // environment? Yes (1) or no (0);
  bool sync;  // If pausing, execution environment must sync/commit time warp
              // store? sync (1) or no sync (0).
  bool tick_delay; // Should the delay be interpreted in ticks (1) or clock
                   // intervals (0).
  Error error;
  Type delay;
};

// AKA Clock
struct Source {
  virtual ~Source() = default;
  virtual Type interval() = 0;
};

// AKA something clocked
struct Listener {
  virtual ~Listener() = default;
  virtual const Source *getSource() = 0;
  virtual void setSource(Source *) = 0;
  virtual Result tick(Tick::Type currentTick) = 0;
};
} // namespace Tick

namespace Memory {
struct Operation {
  bool speculative;
  enum class Kind : bool { instruction = false, data = true } kind;
  bool effectful;
};

enum class Error : quint8 {
  Success = 0,
  Unmapped,  //  Attempted to read a physical address with no device present.
  OOBAccess, //  Attempted out-of-bound access on a storage device.
  NeedsMMI,  //  Attempted to read MMI that had no buffered input.
  Breakpoint
};

struct Result {
  bool completed; // Did the operation complete? Yes (1), or No (0).
  bool advance;   // Should a logic FSM retry the current state? Success (1) or
                  // Retry (0).
  bool pause;     // Should a logic FSM be interrupted at the end of the current
                  // tick? yes (1) or no (0).
  bool sync;   // On pausing, is the device required to sync timewarpstore? sync
               // (1) or no sync (0).
  Error error; // Additionally error information.
};

template <typename Address> struct Interposer {
  enum class Result { Success = 0, Breakpoint };
  ~Interposer() = default;
  virtual Result tryRead(Address address, quint8 length, Operation op) = 0;
  virtual Result tryWrite(Address address, const quint8 *data, quint8 length,
                          Operation op) = 0;
};

template <typename Address> struct Target {
  struct AddressSpan {
    Address minOffset, length;
  };
  virtual ~Target() = default;
  virtual AddressSpan span() const = 0;
  virtual Result read(Address address, quint8 *dest, quint8 length,
                      Operation op) const = 0;
  virtual Result write(Address address, const quint8 *src, quint8 length,
                       Operation op) = 0;
  virtual void clear(quint8 fill) = 0;

  virtual void setInterposer(Interposer<Address> *inter) = 0;
};

template <typename Address> struct Initiator {
  virtual ~Initiator() = default;
  virtual void
  setTarget(Target<Address>
                *target) = 0; // Sets all targets. e.g., both I and D paths.
  virtual void
  setTarget(void *port,
            Target<Address> *target) = 0; // Sets one target within the
                                          // initiator. Datatype undetermined.
};
} // namespace Memory

struct Scheduler {
  enum class Mode {
    Increment, // Execute only the next tick, even if no clocked device is
               // ticked.
    Jump, // Execute up to and including the next tick with a clocked device.
  };
  virtual ~Scheduler() = default;
  virtual Tick::Listener *next(Tick::Type current, Mode mode) = 0;
  virtual void schedule(Tick::Listener *listener, Tick::Type startingOn) = 0;
  virtual void reschedule(Device::ID device, Tick::Type startingOn) = 0;
};

template <typename Address> struct System {
  virtual ~System() = default;
  virtual void addTarget(const Device::Descriptor device,
                         Memory::Target<Address> *target) = 0;
  virtual void addClock(const Device::Descriptor device,
                        Tick::Source *clock) = 0;
  virtual void addClocked(const Device::Descriptor device,
                          Tick::Listener *clocked) = 0;

  // Returns (current tick, result of ticking that clocked device).
  virtual std::pair<Tick::Type, Tick::Result> tick(Scheduler::Mode mode) = 0;
  virtual Tick::Type currentTick() const = 0;
  virtual Device::ID nextID() = 0;

  virtual void setTraceBuffer(Trace::Buffer *buffer) = 0;
};

} // namespace sim::api
