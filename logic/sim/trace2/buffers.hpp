#pragma once
#include "lru/cache.hpp"
#include "sim/api2.hpp"
#include "sim/trace2/packet_utils.hpp"

namespace sim::trace2 {
class IsTraced {
public:
    IsTraced(QSet<sim::api2::device::ID> devices): _devices(devices) {};
    template <HasDevice Header>
    bool operator()(const Header& header) const {
        quint16 val = header.device;
        return _devices.contains(val);
    };
    bool operator()(const auto& header) const {return false;}
private:
    QSet<sim::api2::device::ID> _devices;
};

template <typename Target>
struct IsType {
    bool operator()(const auto& header) {
        return std::same_as<Target, std::remove_cv<decltype(header)>>;
    }
};

class InfiniteBuffer : public api2::trace::Buffer, public api2::trace::IteratorImpl {
public:
    using FrameIterator = api2::trace::FrameIterator;
    InfiniteBuffer();
    // Buffer interface
    bool trace(quint16 deviceID, bool enabled) override;
    bool registerSink(api2::trace::Sink *) override;
    void unregisterSink(api2::trace::Sink *) override;
    bool writeFragment(const api2::frame::Header&) override;
    bool writeFragment(const api2::packet::Header&) override;
    bool writeFragment(const api2::packet::Payload&) override;
    bool updateFrameHeader() override;
    void dropLast() override;
    FrameIterator cbegin() const override;
    FrameIterator cend() const override;
    FrameIterator crbegin() const override;
    FrameIterator crend() const override;

private:
    QSet<api2::device::ID> _traced = {};
    QSet<api2::trace::Sink*> _sinks = {};
    std::size_t _lastFrameStart = 0;
    // Need to be mutable so that IteratorImpl can read from them.
    mutable std::vector<std::byte> _data = {};

    mutable LRU::Cache<std::size_t, std::size_t> _backlinks;
    zpp::bits::in<decltype(_data)> _in;
    zpp::bits::out<decltype(_data)> _out;
    // Like normal next, except you can control if the iterator jumps between frames
    // or walks cell-by-cell. user-facing next calls can take advantage of the speed,
    // while internal next calls can use next to fill backlink cache.
    std::size_t next(std::size_t loc, api2::trace::Level level, bool allow_jumps) const;
    std::size_t last_before(std::size_t start, std::size_t end, api2::trace::Level payload) const;

    // IteratorImpl interface
public:
    std::size_t end() const override;
    std::size_t size_at(std::size_t loc, api2::trace::Level level) const override;
    api2::trace::Level at(std::size_t loc) const override;
    api2::frame::Header frame(std::size_t loc) const override;
    api2::packet::Header packet(std::size_t loc) const override;
    api2::packet::Payload payload(std::size_t loc) const override;
    std::size_t next(std::size_t loc, api2::trace::Level level) const override;
    std::size_t prev(std::size_t loc, api2::trace::Level level) const override;
};
} // namespace sim::trace2
