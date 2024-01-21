#pragma once
#include "bytes.hpp"

namespace trace2::packet {

using device_id_t = zpp::bits::varint<quint16>;
enum class DeltaEncoding : quint8 {
    XOR, // The old and new values are XOR'ed together
};

namespace header {
struct Frame {
    // Number of bytes to the start of the next FrameHeader.
    // The code responsible for starting a new frame needs to
    // go back and update this field.
    // If 0, then this is the last frame in the trace.
    quint16 length = 0;
    // Number of bytes to the start of the previous FrameHeader.
    // If 0, then this is the first frame in the trace.
    zpp::bits::varint<quint16> back_offset = 0;
};

struct Clear {
    device_id_t device = 0;
    Bytes<8> value = {0,0};
};

struct PureRead {
    device_id_t device = 0;
    zpp::bits::varint<quint64> payload_len = 0;
    Bytes<8> address = {0,0};
};

// MUST be followed by 1+ payloads.
struct ImpureRead {
    device_id_t device = 0;
    Bytes<8> address = {0,0};
};

// MUST be followed by 1+ payload.
struct Write {
    device_id_t device = 0;
    Bytes<8> address = {0,0};
};
} // trace2::packet::header
using Header = std::variant<header::Frame, header::PureRead, header::ImpureRead, header::Write>;

namespace payload {
// Successive payloads belong to the same packet.
struct Variable {
    Bytes<32> payload = {0, 0};
};
} // trace2::packet::payload
using Payload = std::variant<payload::Variable>;
using Fragment = std::variant<Header, Payload>;
} //trace2::packet
