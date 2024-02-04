#pragma once

#include "sim/api2.hpp"
#include "bits/operations/copy.hpp"
namespace sim::trace2 {
template<typename T>
concept HasLength = requires(T t) {
    {
        t.length
    } -> std::convertible_to<decltype(t.length)>;
};

class UpdateFrameLength
{
public:
    UpdateFrameLength(quint16 length, sim::api2::frame::Header &out)
        : _length(length)
        , _out(out){};
    template<HasLength Header>
    void operator()(const Header &header)
    {
        Header copy = header;
        copy.length = _length;
        _out = copy;
    }
    void operator()(const auto &header){};

private:
    quint16 _length = 0;
    sim::api2::frame::Header &_out;
};

struct GetFrameLength
{
    template<HasLength Header>
    quint16 operator()(const Header &header) const
    {
        return header.length;
    }
    quint16 operator()(const auto &header) const { return 0; };
};

template<typename T>
concept HasBackOffset = requires(T t) {
    {
        t.back_offset
    } -> std::convertible_to<decltype(t.back_offset)>;
};
class UpdateFrameBackOffset
{
public:
    UpdateFrameBackOffset(quint16 back_offset, sim::api2::frame::Header &out)
        : _back_offset(back_offset)
        , _out(out){};
    template<HasBackOffset Header>
    void operator()(const Header &header)
    {
        Header copy = header;
        copy.back_offset = _back_offset;
        _out = copy;
    }
    void operator()(const auto &header){};

private:
    quint16 _back_offset = 0;
    sim::api2::frame::Header &_out;
};

struct GetFrameBackOffset
{
    template<HasBackOffset Header>
    quint16 operator()(const Header &header) const
    {
        return header.back_offset;
    }
    quint16 operator()(const auto &header) const { return 0; };
};
} // namespace sim::trace2
