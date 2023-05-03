#pragma once
#include <QtCore>
#include <type_traits>
namespace sim::api {

namespace device {
using ID = quint16; // Only use 9 bits (max of 512)!
struct Descriptor {
  device::ID id;
  void *compatible;
  QString baseName, fullName;
};
using IDGenerator = std::function<ID()>;
} // namespace device

namespace packet {
// clang-format off
/*
 * Flags allow a device to cast a memory location to the correct kind of Packet
 * by encoding type information.
 * kind:
 *   When scope==0, all packets with the same kind bits must have identical
 *   storage layouts. When scope==1, only packets belonging to the same device
 *   must have the same storage layout.
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
 * u16:
 *   0: treat the flags as a 1-bytes value.
 *   1: treat the flags as a 2-byte value.
 * Special values for bits :
 *   0b0000'0000 indicates and empty Packet that is size 1.
 *   0bxxxx'xxxx'xxxx'xxx1 indicates the flags should be treated as a u16.
 *
 * Bits are orgnized so that all common addresses traces can fit in 0bxxx'xxx'0'0,
 * where at least one x is 1. This does mean that only 2-byte flags can hold
 * dynamic data.
 */
// clang-format on
struct Flags {
  Flags() = default;
  quint16 dyn : 1 =
      0; // 1=some data allocated with malloc/new. Must be destroyed.
  quint16 kind : 13 = 0;
  quint16 scope : 1 = 0; // 0=global, 1=specific to device;
  quint16 u16 : 1 = 0;   // Must always be 1 if flags is non-zero. Indicate that
                         // flags take 2 bytes, not 1.
  inline operator quint16() const {
    union Type {
      Type() : flags() {}
      Flags flags;
      quint16 bits;
    } type;
    type.flags = *this;
    return type.bits;
  }
};
static_assert(sizeof(Flags) == sizeof(quint16));

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
  // Device must be after payload, so it is at a fixed location relative to
  // type.
  //  [typdevice, type] are used to compute length if a pointer to the end of
  //  the packet is acquired.
  device::ID device = 0;
  // Flags are always stored as u16. If u16 bit is not set, then the upper 8
  // bits are unspecified.
  union {
    Flags flags;
    quint16 bits =
        0b0000'0000'0000'0001; // Mark the type as 2 bytes, uninitialized.
  } type;

  Packet() {}
  Packet(device::ID id, Flags flags) : device(device), payload(), type(flags) {}
  // Must be declared inline, otherwise fails to compile.
  template <typename Bytes>
  Packet(device::ID device, Bytes bytes, Flags flags)
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
} // namespace packet

namespace trace {

// Forward declare, will be needed inside analyzer.
struct Buffer;

struct Analyzer {
  virtual ~Analyzer() = 0;
};
Analyzer::~Analyzer() = default;

// Ex
struct Buffer {
  using AnalyzerHookID = quint32;
  virtual ~Buffer() = default;
  // To avoid double-buffering, request that the buffer provide a sufficient
  // number of bytes. Callers can then use placement new to construct their
  // Packet in-place. If return is nullptr, do not attempt to perform
  // placement.
  // Throws a bad alloc exception if there is no space in buffer.
  virtual void *alloc(quint8 length) = 0;
  virtual void trace(quint16 deviceID, bool enabled = true) = 0;
  virtual void setPacketRegistry(api::packet::Registry *registry) = 0;

  [[nodiscard]] virtual AnalyzerHookID registerAnalyzer(Analyzer *analyzer) = 0;
  [[nodiscard]] virtual Analyzer *unregisterAnalyzer(AnalyzerHookID id) = 0;
};

struct Producer {
  virtual ~Producer() = default;
  virtual void setTraceBuffer(Buffer *tb) = 0;
  // Have the produce communicate with the Buffer that it would like to traced.
  // In the case of a CPU with register banks, calling trace() should cause the
  // CPU to make its register banks trace()'ed too.
  virtual void trace(bool enabled) = 0;
  virtual quint8 packetSize(packet::Flags flags) const = 0;
  virtual bool applyTrace(void *trace) = 0;   // trace is a Packet struct.
  virtual bool unapplyTrace(void *trace) = 0; // trace is a Packet struct.
};
} // namespace trace

namespace tick {
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
  virtual Result tick(tick::Type currentTick) = 0;
};
} // namespace tick

namespace memory {
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
} // namespace memory

struct Scheduler {
  enum class Mode {
    Increment, // Execute only the next tick, even if no clocked device is
               // ticked.
    Jump, // Execute up to and including the next tick with a clocked device.
  };
  virtual ~Scheduler() = default;
  virtual tick::Listener *next(tick::Type current, Mode mode) = 0;
  virtual void schedule(tick::Listener *listener, tick::Type startingOn) = 0;
  virtual void reschedule(device::ID device, tick::Type startingOn) = 0;
};

template <typename Address> struct System {
  virtual ~System() = default;
  virtual void addTarget(const device::Descriptor device,
                         memory::Target<Address> *target) = 0;
  virtual void addClock(const device::Descriptor device,
                        tick::Source *clock) = 0;
  virtual void addClocked(const device::Descriptor device,
                          tick::Listener *clocked) = 0;

  // Returns (current tick, result of ticking that clocked device).
  virtual std::pair<tick::Type, tick::Result> tick(Scheduler::Mode mode) = 0;
  virtual tick::Type currentTick() const = 0;
  virtual device::ID nextID() = 0;
  virtual device::IDGenerator nextIDGenerator() = 0;

  virtual void setTraceBuffer(trace::Buffer *buffer) = 0;
};

} // namespace sim::api
