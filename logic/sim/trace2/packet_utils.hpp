#pragma once
#include "sim/api2.hpp"
namespace sim::trace2 {
// TODO: should check that the struct is a packet header.
template <typename T>
concept HasDevice =
    requires(T t) {
    { t.device } -> std::same_as<sim::api2::device::ID>;
};
class IsSameDevice {
public:

    IsSameDevice(sim::api2::device::ID device): _device(device) {};
    template <HasDevice Header>
    bool operator()(const Header& header) const {return header.device == _device;};
    bool operator()(const auto& header) const {return false;}
private:
    sim::api2::device::ID _device;
};
template <typename Address>
void emitWrite(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
           Address address, bits::span<const quint8> src, bits::span<quint8> dest) {
  // TODO: Emit a write packet header followed by payload fragments
}

// Generate a Write packet. Bytes will not be XOR encoded.
// A write to a MM port appends to the state of that port.
// We do not need to know previous value, since the pub/sub system records it.
template <typename Address>
void emitMMWrite(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
               Address address, bits::span<const quint8> src) {
    // TODO: Emit a write packet header followed by payload fragments
}


template <typename Address>
void emitPureRead(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
           Address address, Address len) {
  // TODO: Emit a pure read header.
}

// Generate a impureRead packet. Bytes will not be XOR encoded.
// We do not need to know previous value, since the pub/sub system records it.
template <typename Address>
void emitMMRead(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
                 Address address, bits::span<const quint8> src) {
    // TODO: Emit a write packet header followed by payload fragments
}


}
