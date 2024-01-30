#include "buffers.hpp"
#include "frame_utils.h"

// Wrap each fragment is a top-level variant.
// This makes "switching" on the underlying fragment type
// less difficult to implement. It adds an overhead of
// ~1 byte per fragment to the output stream.
using wrapped = std::variant<sim::api2::frame::Header, sim::api2::packet::Header, sim::api2::packet::Payload>;

sim::trace2::InfiniteBuffer::InfiniteBuffer(): _in(_data), _out(_data)
{
}

bool sim::trace2::InfiniteBuffer::trace(sim::api2::device::ID deviceID, bool enabled)
{
    if(enabled) _traced.insert(deviceID);
    else _traced.remove(deviceID);
    return true;
}

bool sim::trace2::InfiniteBuffer::registerSink(api2::trace::Sink *sink,
                                               api2::trace::Mode /*mode*/)
{
    if(_sinks.contains(sink)) return false;
    _sinks.insert(sink);
    return true;
}

void sim::trace2::InfiniteBuffer::unregisterSink(api2::trace::Sink * sink)
{
    _sinks.remove(sink);
}

bool sim::trace2::InfiniteBuffer::writeFragment(const api2::frame::Header& header)
{
    // Both will point to same position when header is first element to be
    // serialized.
    if(_lastFrameStart  != _out.position()) updateFrameHeader();

    // Zero out length field of header, and set back_offset.
    api2::frame::Header hdr = header;
    std::visit(sim::trace2::UpdateFrameLength{0}, hdr);
    quint16 back_offset = _lastFrameStart - _out.position();
    std::visit(sim::trace2::UpdateFrameBackOffset{back_offset}, hdr);

    // Save current offset to enable updateFrameHeader() to overwrite length in the future.
    _lastFrameStart = _out.position();

    _out(wrapped(hdr)).or_throw();
    return true;
}

bool sim::trace2::InfiniteBuffer::writeFragment(const api2::packet::Header& header)
{
    // If device is not traced, do not record the packet.
    if(! std::visit(IsTraced(_traced), header)) return false;
    _out(wrapped(header)).or_throw();
    return true;
}

bool sim::trace2::InfiniteBuffer::writeFragment(const api2::packet::Payload& payload)
{
    _out(wrapped(payload)).or_throw();
    return true;
}

bool sim::trace2::InfiniteBuffer::updateFrameHeader()
{
    namespace fh = api2::frame::header;
    using Header = api2::frame::Header;
    auto curOutPos = _out.position();
    auto curInPos = _in.position();

    // Read in previous frame header and update its length flag
    wrapped w;
    _in.reset(_lastFrameStart);
    _in(w).or_throw();
    _in.reset(curInPos);

    if(!std::holds_alternative<Header>(w)) return false;
    Header hdr = std::get<Header>(w);

    // TODO: Ensure that length fits in 16 bits.
    quint32 length = curOutPos - _lastFrameStart;
    std::visit(sim::trace2::UpdateFrameLength{static_cast<quint16>(length)}, hdr);

    // Overwrite existing frame header to update "length" field.
    _out.reset(_lastFrameStart);
    _out(hdr).or_throw();
    _out.reset(curOutPos);

    return true;
}

void sim::trace2::InfiniteBuffer::dropLast()
{

}

sim::trace2::InfiniteBuffer::TraceIterator sim::trace2::InfiniteBuffer::cbegin() const
{
    return TraceIterator(this, 0);
}

sim::trace2::InfiniteBuffer::TraceIterator sim::trace2::InfiniteBuffer::cend() const
{
    return TraceIterator(this, _out.position());
}

sim::api2::trace::Buffer::TraceIterator sim::trace2::InfiniteBuffer::crbegin() const
{
    return TraceIterator(this, _out.position(), TraceIterator::Reverse);
}

sim::trace2::InfiniteBuffer::TraceIterator sim::trace2::InfiniteBuffer::crend() const
{
    return TraceIterator(this, 0, TraceIterator::Reverse);
}

std::size_t sim::trace2::InfiniteBuffer::size_at(std::size_t loc, api2::trace::Level level) const
{
    typename std::remove_const<decltype(_in)>::type in(_data);
    in.reset(loc);

    wrapped w;
    in(w).or_throw();

    return in.position() - loc;
}

sim::api2::trace::Level sim::trace2::InfiniteBuffer::at(std::size_t loc) const
{
    typename std::remove_const<decltype(_in)>::type in(_data);
    in.reset(loc);

    wrapped w;
    in(w).or_throw();

    if(std::holds_alternative<api2::frame::Header>(w))
      return api2::trace::Level::Frame;
    if(std::holds_alternative<api2::packet::Header>(w))
      return api2::trace::Level::Packet;
    else
      return api2::trace::Level::Payload;
}

sim::api2::frame::Header sim::trace2::InfiniteBuffer::frame(std::size_t loc) const
{
    typename std::remove_const<decltype(_in)>::type in(_data);
    in.reset(loc);

    wrapped w;
    in(w).or_throw();
    return std::get<sim::api2::frame::Header>(w);
}

sim::api2::packet::Header sim::trace2::InfiniteBuffer::packet(std::size_t loc) const
{
    typename std::remove_const<decltype(_in)>::type in(_data);
    in.reset(loc);

    wrapped w;
    in(w).or_throw();
    return std::get<sim::api2::packet::Header>(w);
}

sim::api2::packet::Payload sim::trace2::InfiniteBuffer::payload(std::size_t loc) const
{
    typename std::remove_const<decltype(_in)>::type in(_data);
    in.reset(loc);

    wrapped w;
    in(w).or_throw();
    return std::get<sim::api2::packet::Payload>(w);
}

std::size_t sim::trace2::InfiniteBuffer::next(std::size_t loc, api2::trace::Level level) const
{
    typename std::remove_const<decltype(_in)>::type in(_data);
    loc += size_at(loc, level);
    in.reset(loc);

    wrapped w;
    do {
        if(loc == _out.position()) return loc;
        loc = in.position();
        auto ret = in(w);
        if(ret.code == std::errc::result_out_of_range) return 0;
        else if(ret.code != std::errc{}) throw std::logic_error("Unhandled");
        // Prevent "going up" to the next level of trace by returning 0.
        switch(level) {
        case api2::trace::Level::Frame:
            if(std::holds_alternative<sim::api2::frame::Header>(w)) return loc;
            break;
        case api2::trace::Level::Packet:
            if(std::holds_alternative<sim::api2::frame::Header>(w)
                || std::holds_alternative<sim::api2::packet::Header>(w)) return loc;
            break;
        case api2::trace::Level::Payload:
            if(std::holds_alternative<sim::api2::frame::Header>(w)
                || std::holds_alternative<sim::api2::packet::Header>(w)
                || std::holds_alternative<sim::api2::packet::Payload>(w)) return loc;
            break;
        }
        loc = in.position();
    } while (true);
}

std::size_t sim::trace2::InfiniteBuffer::prev(std::size_t loc, api2::trace::Level level) const
{
    // Frame: loc should always point to a header, so we follow the back_offset
    //   Create a cached LUT to map index <=> packet locations for this frame.
    //   Track which range where the LUT applies.
    // Packet: if LUT applies, use it to find the previous packet.
    //   Othewise, scan forwards until we hit a frame header.
    //   If we hit end without hitting a header, search from _lastFrameStart.
    //   Construct a cacheable LUT entry for the payloads.
    // Payload: if LUT applies, find previous payload.
    //   Otherwise, perform packet scan, and try again.
    throw std::logic_error("Unimplemented");
}
