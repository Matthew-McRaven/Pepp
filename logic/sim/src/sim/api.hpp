#pragma once
#include <QtCore>
#include <type_traits>

namespace sim::api {
using DeviceID = quint16; // Only use 9 bits (max of 512)!

struct Device {
  virtual ~Device() = 0;
  virtual QString baseName() const = 0;
  virtual DeviceID id() const = 0;
  virtual void *comptabible() const = 0;
};

namespace Packet {
struct Flags {
  quint16 delta : 1; // 0=stats, 1=delta;
  quint16 deviceKind : 9;
  quint16 dataKind : 6;
  inline operator quint16() const {
    union {
      Flags flags;
      quint16 bits;
    } type;
    type.flags = *this;
    return type.bits;
  }
};
static_assert(sizeof(Flags) == sizeof(quint16));

#pragma pack(push, 1)
template <typename Payload> struct Packet {
  Packet(DeviceID device, Payload payload, Flags flags)
      : device(device), payload(payload), type(flags) {}
  quint8 length = sizeof(Packet);
  DeviceID device;
  Payload payload;
  union {
    Flags flags;
    quint16 bits;
  } type;
};
#pragma pack(pop)

struct Registry {
  typedef void (*packet_dtor)(void *);
  void registerDTOR(Flags flags, packet_dtor dtor);
  packet_dtor getDTOR(Flags flags); // nullptr indicates no DTOR.
};
} // namespace Packet

namespace Trace {

// Forward declare, will be needed inside analyzer.
struct Buffer;

struct Analyzer {
  virtual ~Analyzer() = 0;
};

struct Buffer {
  using AnalyzerHookID = quint32;
  enum class Status : quint8 {
    Success = 0,        // Object is now pending.
    OverflowAndSuccess, // Operation succeeded, but the next operation might
                        // not. Sync the buffer. Object IS pending.
    OverflowAndRetry,   // Operation did not succeed. Retry current step after
                        // sync'ing buffer. Object is NOT pending.
  };
  virtual ~Buffer() = 0;
  virtual Status push(void *trace) = 0; // "trace" is a Packet struct.
  virtual void trace(quint16 deviceID, bool enabled = true);
  virtual void setPacketRegistry(Packet::Registry *registry);

  [[nodiscard]] virtual AnalyzerHookID registerAnalyzer(Analyzer *analyzer);
  [[nodiscard]] virtual Analyzer *unregisterAnalyzer(AnalyzerHookID id);
};

struct Producer {
  virtual bool setTraceBuffer(Buffer *tb) = 0;
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
  virtual ~Source() = 0;
  virtual Type interval() = 0;
};

// AKA something clocked
struct Listener {
  virtual ~Listener() = 0;
  virtual const Source *getSource() = 0;
  virtual void setSource(Source *) = 0;
  virtual Result tick(Tick::Type currentTick) = 0;
};
} // namespace Tick

namespace Memory {
struct Operation {
  enum class speculative : bool { no = false, yes = true } speculative;
  enum class kind : bool { instruction = false, data = true } kind;
  enum class effectful : bool { no = false, yes = true } effectful;
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

  Result tryRead(Address address, quint8 length, Operation op);
  Result tryWrite(Address address, const quint8 *data, quint8 length,
                  Operation op);
};

template <typename Address> struct Target {
  struct AddressSpan {
    Address base, length;
  };
  virtual ~Target() = 0;
  virtual AddressSpan span() const = 0;
  virtual Result read(Address address, quint8 *dest, quint8 length,
                      Operation op) = 0;
  virtual Result write(Address address, const quint8 *src, quint8 length,
                       Operation op) = 0;
  virtual void clear(quint8 fill) = 0;

  virtual void setTraceBuffer(Trace::Buffer *tb) = 0;
  virtual void setInterposer(Interposer<Address> *inter) = 0;
};

template <typename Address> struct Initiator {
  virtual ~Initiator() = 0;
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
  virtual ~Scheduler() = 0;
  virtual Tick::Listener *next(Tick::Type current, Mode mode) = 0;
  virtual void schedule(Tick::Listener *listener, Tick::Type startingOn) = 0;
  virtual void reschedule(DeviceID device, Tick::Type startingOn) = 0;
};

template <typename Address> struct System {
  virtual ~System() = 0;
  virtual void addTarget(Memory::Target<Address> *target) = 0;
  virtual void addClock(Tick::Source *clock) = 0;
  virtual void addClocked(Tick::Listener *clocked) = 0;

  // Returns (current tick, result of ticking that clocked device).
  virtual std::pair<Tick::Type, Tick::Result> tick(Scheduler::Mode mode) = 0;
  virtual Tick::Type currentTick() const = 0;
  virtual DeviceID nextID() = 0;

  virtual void setTraceBuffer(Trace::Buffer *buffer) = 0;
};

} // namespace sim::api
