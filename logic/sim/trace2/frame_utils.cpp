#include "frame_utils.h"
void sim::trace2::UpdateFrameLength::operator()(api2::frame::header::Trace &header) const
{
    header.length = _length;
}

quint16 sim::trace2::GetFrameLength::operator()(api2::frame::header::Trace &header) const
{
    return header.length;
}

void sim::trace2::UpdateFrameBackOffset::operator()(api2::frame::header::Trace &header) const
{
    header.back_offset = _back_offset;
}

quint16 sim::trace2::GetFrameBackOffset::operator()(api2::frame::header::Trace &header) const
{
    return header.back_offset;
}
