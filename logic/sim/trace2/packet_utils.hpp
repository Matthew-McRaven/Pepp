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
}
