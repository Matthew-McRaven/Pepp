#pragma once
#include "packet.hpp"

namespace trace2 {
struct Buffer;
struct Source {
    virtual void setBuffer(Buffer* tb) = 0;
    virtual void trace(bool enabled) = 0;
};

struct Sink {
    enum class Direction {
        Forward,    // Apply the action specified by the packet.
        Backward,   // Undo the effects of the action specified by the packet.
    };
    virtual bool filter(const packet::Header&) = 0;
    virtual bool analyze(const packet::Header&, const std::span<packet::Payload>&, int direction=0) = 0;
};
} // namespace trace2
