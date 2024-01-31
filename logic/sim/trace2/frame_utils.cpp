#include "frame_utils.h"
void sim::trace2::UpdateFrameLength::operator()(const api2::frame::header::Trace &header) const
{
    api2::frame::header::Trace copy = header;
    copy.length = _length;
    _out = copy;
}

quint16 sim::trace2::GetFrameLength::operator()(const api2::frame::header::Trace &header) const
{
    return header.length;
}

void sim::trace2::UpdateFrameBackOffset::operator()(const api2::frame::header::Trace &header) const
{
    api2::frame::header::Trace copy = header;
    copy.back_offset = _back_offset;
    _out = copy;
}

quint16 sim::trace2::GetFrameBackOffset::operator()(const api2::frame::header::Trace &header) const
{
    return header.back_offset;
}
