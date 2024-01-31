#pragma once

#include "sim/api2.hpp"
#include "bits/operations/copy.hpp"
namespace sim::trace2 {
class UpdateFrameLength {
public:
    UpdateFrameLength(quint16 length)
        : _length(length){};
    void operator()(sim::api2::frame::header::Trace &header) const;
    void operator()(auto &header) const {};

private:
    quint16 _length = 0;
};

struct GetFrameLength
{
    quint16 operator()(sim::api2::frame::header::Trace &header) const;
    quint16 operator()(auto &header) const { return 0; };
};

class UpdateFrameBackOffset {
public:

    UpdateFrameBackOffset(quint16 back_offset): _back_offset(back_offset) {};
    void operator()(sim::api2::frame::header::Trace &header) const;
    void operator()(auto &header) const {};

private:
    quint16 _back_offset = 0;
};
struct GetFrameBackOffset
{
    quint16 operator()(sim::api2::frame::header::Trace &header) const;
    quint16 operator()(auto &header) const { return 0; };
};
} // sime::trace2
