#pragma once
#include <QtCore>
#include <zpp_bits.h>

#include "bits/span.hpp"

namespace sim::api2 {

static const int VERSION = 2;

namespace device {
using ID = quint16; // Only use 9 bits (max of 512)!
struct Descriptor {
    device::ID id; // Must uniquely identify a device within a system.
    void *compatible;
    QString baseName, fullName;
};
using IDGenerator = std::function<ID()>;
} // namespace sim::api2::device


namespace packet {
// Stack-allocated variable length array of bytes. When serialized, it will only occupy
// the number of bytes specified in data_len, not the size as specified by N.
// One use for this class is to serialize addresses, which may be 1-8 bytes long.
// These variable-length addresses allow us to use the same packet for all memory accesses.
// For example, in Pep/10 a register is a 1 byte address, where main memory is 2.
// Our old trace system needed a packet type for each address size.
// The variable-length byte-array allows this to be expressed in a single packet type.
template <size_t N>
struct VariableBytes {
    // TODO: add helper methods to convert to/from  un/signed int8/16/32/64 in either endianness.
    constexpr static zpp::bits::errc serialize(auto& archive, auto& self) {
        // Serialize array manually to avoid allocating extra 0's in the bit stream.
        // Must also de-serialize manually, otherwise archive will advance the position
        // by the allocated size of the array, not the "used" size.
        if(archive.kind() == zpp::bits::kind::out) {

            if(self.len > N) return zpp::bits::errc(std::errc::value_too_large);
            zpp::bits::errc errc = archive(self.len);
            if(errc.code != std::errc()) return errc;
            else if(self.len == 0) return errc;

            // We serialized the length ourselves. If we pass array_view directly, size will be serialzed again.
            auto array_view = std::span<quint8>(self.bytes.data(), self.len);
            return archive(zpp::bits::bytes(array_view, array_view.size_bytes()));
        } else {
            zpp::bits::errc errc = archive(self.len);
            if(errc.code != std::errc()) return errc;
            else if(self.len > N) return zpp::bits::errc(std::errc::value_too_large);
            else if(self.len == 0) return errc;

            auto array_view = std::span<quint8>(self.bytes.data(), self.len);
            // See above.
            return archive(zpp::bits::bytes(array_view, array_view.size_bytes()));
        }
    }
    quint8 len = 0;
    std::array<quint8, N> bytes = {0};
};

using device_id_t = zpp::bits::varint<quint16>;
enum class DeltaEncoding : quint8 {
    XOR, // The old and new values are XOR'ed together
};

namespace header {
struct Frame {
    // Number of bytes to the start of the next FrameHeader.
    // The code responsible for starting a new frame needs to
    // go back and update this field.
    // If 0, then this is the last frame in the trace.
    quint16 length = 0;
    // Number of bytes to the start of the previous FrameHeader.
    // If 0, then this is the first frame in the trace.
    zpp::bits::varint<quint16> back_offset = 0;
};

struct Clear {
    device_id_t device = 0;
    VariableBytes<8> value = {0,0};
};

struct PureRead {
    device_id_t device = 0;
    zpp::bits::varint<quint64> payload_len = 0;
    VariableBytes<8> address = {0,0};
};

// MUST be followed by 1+ payloads.
struct ImpureRead {
    device_id_t device = 0;
    VariableBytes<8> address = {0,0};
};

// MUST be followed by 1+ payload.
struct Write {
    device_id_t device = 0;
    VariableBytes<8> address = {0,0};
};
} // sim::api2::packet::header
using Header = std::variant<header::Frame, header::PureRead, header::ImpureRead, header::Write>;

namespace payload {
// Successive payloads belong to the same packet.
struct Variable {
    VariableBytes<32> payload = {0, 0};
};
} // sim::api2::packet::payload
using Payload = std::variant<payload::Variable>;
using Fragment = std::variant<Header, Payload>;

// Elements are "Packet"'s.
struct Iterator {
    virtual void next() = 0;
};
} //sim::api2::packet

struct Packet {
    const packet::Header* header;
    std::span<const packet::Payload> payload;
};

struct Frame {
    virtual packet::header::Frame frameHeader() = 0;
    virtual packet::Iterator begin() = 0;
    virtual packet::Iterator end() = 0;
};


namespace frame {
// Elements are "Frame"'s. Each frame can be iterated over to get its packets.
struct Iterator {
    // Move to next frame.
    virtual void next() = 0;
};
} // namespace sim::api2::frame

namespace trace {

enum class Mode {
    Realtime, // Trace frames are parsed as they are received
    Deferred, // Trace frames will be parsed at some later point.
};

class Buffer;
class Source {
public:
    virtual ~Source() = 0;
    virtual void setBuffer(Buffer* tb) = 0;
    virtual void trace(bool enabled) = 0;
};

class Sink {
public:
    virtual ~Sink() = 0;
    enum class Direction {
        Forward,    // Apply the action specified by the packet.
        Backward,   // Undo the effects of the action specified by the packet.
    };
    virtual bool filter(const packet::Header&) = 0;
    virtual bool analyze(const packet::Header&, const std::span<packet::Payload>&, Direction) = 0;
};

class Buffer {
public:
    virtual ~Buffer() = 0;
    virtual bool trace(quint16 deviceID, bool enabled = true) = 0;

    virtual bool registerSink(Sink*, Mode) = 0;
    virtual void unregisterSink(Sink*) = 0;

    virtual bool writeFragment(const packet::Header&) = 0;
    virtual bool writeFragment(const packet::Payload&) = 0;

    // Start a new frame by inserting a frame header packet.
    virtual bool startFrame() = 0;

    // Update frame header back pointer and size. Probably called from the clock source?
    virtual bool updateFrame() = 0;

    // Remove the last frame from the buffer.
    // TODO: replace with integration for iterators / std::erase.
    virtual void dropLast() = 0;


    virtual frame::Iterator rbegin() const = 0;
    virtual frame::Iterator rend() const = 0;
    virtual frame::Iterator begin() const = 0;
    virtual frame::Iterator end() const = 0;
};
} // namespace sim::api2::trace

// In API v1, errors are communicated via an Error field.
// In API v2 (this version), errors are communicated via exceptions.
namespace tick {
using Type = quint32;
struct Result {
    // After this tick, should control be returned to execution enviroment?
    bool pause = false;
    // Number of ticks required before this device can be clock'ed again.
    // If you need a 1 cycle delay, your delay would be 1*source->interval();
    // 2 tick delay is 1*source->interval(), etc.
    Type delay;
};

class Source {
public:
    virtual ~Source() = 0;
    // Number of ticks between clock cycles.
    virtual Type interval() const = 0;
};

class Recipient{
public:
    virtual ~Recipient() = 0;
    virtual const Source* getSource() = 0;
    virtual void setSource(Source*) = 0;
    virtual Result clock(tick::Type currentTick);
};
} // namespace sim::api2::tick

// In API v1, errors are communicated via an Error field.
// In API v2 (this version), errors are communicated via exceptions.
namespace memory {
struct Result {
    // Number of simulation ticks required to complete the memory op.
    // 0 indicates the operation completed immediately.
    tick::Type delay;
    // Did the memory access trigger a breakpoint?
    bool pause;
};

// In API v1, there was a concept of effectful and speculative.
// effectful=false implied that the memory operation was triggered by the UI.
// speculative=true implied some form of memory pre-fetch triggered by the simulated hardware.
// In no case was effectful=false, speculative=true a meaningful combination.
//
// In API v2, these have been condensed into a Operation::Type, eliminating the impossible case.
struct Operation {
    enum class Type : quint8 {
        // Access triggered by the application / user interface.
        // Should not trigger memory-mapped IO, cache misses, etc.
        Application = 1,
        // Speculative access triggered within the simulation. Probably shouldn't trigger
        // MMIO, but this is hardware dependent.
        Speculative = 2,
        // Non-speculative access triggered within  the simulation. Should trigger memory mapped IO,
        // cache updates, etc.
        Standard = 0,
    } type = Type::Standard;
    enum class Kind : bool { instruction = false, data = true } kind;
};

template <typename Address> struct Target {
    struct AddressSpan {
        Address minOffset, maxOffset;
    };
    virtual ~Target() = default;
    virtual AddressSpan span() const = 0;
    virtual Result read(Address address, bits::span<quint8> dest,
                        Operation op) const = 0;
    virtual Result write(Address address, bits::span<const quint8> src,
                         Operation op) = 0;
    virtual void clear(quint8 fill) = 0;

    // If dest is larger than maxOffset-minOffset+1, copy bytes from this target to the span.
    virtual void dump(bits::span<quint8> dest) const = 0;
};

template <typename Address> struct Initiator {
    virtual ~Initiator() = default;
    // Sets the memory backing for a particular port (i.e., set the I and D caches separately)
    // If port is nullptr, then all ports will use the target,
    virtual void setTarget(Target<Address> *target, void* port=nullptr) = 0;
};
} // namespace sim::api2::memory

struct Scheduler {
    enum class Mode {
        Increment, // Execute only the next tick, even if no clocked device is
        // ticked.
        Jump, // Execute up to and including the next tick with a clocked device.
    };
    virtual ~Scheduler() = default;
    virtual tick::Recipient *next(tick::Type current, Mode mode) = 0;
    virtual void schedule(tick::Recipient *listener, tick::Type startingOn) = 0;
    virtual void reschedule(device::ID device, tick::Type startingOn) = 0;
};

template <typename Address> struct System {
    virtual ~System() = default;
    // Returns (current tick, result of ticking that clocked device).
    // Implementors are responsible for creating a frame packet in the TB at the start of the function,
    // and updating the frame header packer at the end of the function.
    // TODO: clock all devices who are scheduled for the next tick.
    virtual std::pair<tick::Type, tick::Result> tick(Scheduler::Mode mode) = 0;
    virtual tick::Type currentTick() const = 0;
    virtual device::ID nextID() = 0;
    virtual device::IDGenerator nextIDGenerator() = 0;

    virtual void setTraceBuffer(trace::Buffer *buffer) = 0;
};

} // namespace sim::api2
