#pragma once

#include "sim/api2.hpp"
#include "bits/operations/copy.hpp"
namespace sim::trace2 {
class UpdateFrameLength {
public:
    UpdateFrameLength(quint16 length, sim::api2::frame::Header &out)
        : _length(length)
        , _out(out){};
    void operator()(const sim::api2::frame::header::Trace &header) const;
    void operator()(const auto &header) const {};

private:
    quint16 _length = 0;
    sim::api2::frame::Header &_out;
};

struct GetFrameLength
{
    quint16 operator()(const sim::api2::frame::header::Trace &header) const;
    quint16 operator()(const auto &header) const { return 0; };
};

class UpdateFrameBackOffset {
public:
    UpdateFrameBackOffset(quint16 back_offset, sim::api2::frame::Header &out)
        : _back_offset(back_offset)
        , _out(out){};
    void operator()(const sim::api2::frame::header::Trace &header) const;
    void operator()(const auto &header) const {};

private:
    quint16 _back_offset = 0;
    sim::api2::frame::Header &_out;
};
struct GetFrameBackOffset
{
    quint16 operator()(const sim::api2::frame::header::Trace &header) const;
    quint16 operator()(const auto &header) const { return 0; };
};
} // sime::trace2
