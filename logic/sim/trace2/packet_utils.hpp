#pragma once
#include "sim/api2.hpp"
#include "bits/operations/copy.hpp"
namespace sim::trace2 {
// TODO: should check that the struct is a packet header.
template <typename T>
concept HasDevice =
    requires(T t) {
    { t.device } -> std::convertible_to<decltype(t.device)>;
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
namespace detail {
void emit_payloads(sim::api2::trace::Buffer* tb,
                   bits::span<const quint8> buf1, bits::span<const quint8> buf2);
void emit_payloads(sim::api2::trace::Buffer* tb,
                   bits::span<const quint8> buf1);
} // sim::trace2::detail

void emitFrameStart(sim::api2::trace::Buffer* tb);

template <typename Address>
void emitWrite(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
           Address address, bits::span<const quint8> src, bits::span<quint8> dest) {
    using vb = decltype(api2::packet::header::Write::address);
    auto address_bytes = vb::from_address<Address>(address);
    auto header = api2::packet::header::Write{.device = id, .address = address_bytes};
    // Don't write payloads if the buffer rejected the packet header.
    if(tb->writeFragment({header})) detail::emit_payloads(tb, src, dest);
}

// Generate a Write packet. Bytes will not be XOR encoded.
// A write to a MM port appends to the state of that port.
// We do not need to know previous value, since the pub/sub system records it.
template <typename Address>
void emitMMWrite(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
               Address address, bits::span<const quint8> src) {
    using vb = decltype(api2::packet::header::Write::address);
    auto header = api2::packet::header::Write{.device = id,
                                              .address = vb::from_address(address)};
    // Don't write payloads if the buffer rejected the packet header.
    if(tb->writeFragment({header})) detail::emit_payloads(tb, src);
}

template <typename Address>
void emitPureRead(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
           Address address, Address len) {
    using vb = decltype(api2::packet::header::PureRead::address);
    auto header = api2::packet::header::PureRead{.device = id, .payload_len = len,
                                                 .address = vb::from_address(address)};
    tb->writeFragment({header});
}

// Generate a ImpureRead packet. Bytes will not be XOR encoded.
// We do not need to know previous value, since the pub/sub system records it.
template <typename Address>
void emitMMRead(sim::api2::trace::Buffer* tb, sim::api2::device::ID id,
                 Address address, bits::span<const quint8> src) {
    using vb = decltype(api2::packet::header::Write::address);
    auto header = api2::packet::header::ImpureRead{.device = id,
                                              .address = vb::from_address(address)};
    // Don't write payloads if the buffer rejected the packet header.
    if(tb->writeFragment({header})) detail::emit_payloads(tb, src);
}

} // sim::trace2
