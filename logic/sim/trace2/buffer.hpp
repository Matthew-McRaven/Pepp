#pragma once
#include <qtypes.h>
#include "packet.hpp"
namespace trace2 {
enum class Mode {
    Realtime, // Trace frames are parsed as they are received
    Deferred, // Trace frames will be parsed at some later point.
};
struct Sink;

struct Packet {
    const packet::Header* header;
    std::span<const packet::Payload> payload;
};

// Elements are "Packet"'s.
struct PacketIterator {
    virtual void next() = 0;
};

struct Frame {
    virtual packet::header::Frame frameHeader() = 0;
    // Return a packet iterator
    virtual PacketIterator begin() = 0;
    virtual PacketIterator end() = 0;
};

// Elements are "Frame"'s. Each frame can be iterated over to get its packets.
struct FrameIterator {
    // Move to next frame.
    virtual void next() = 0;
};

struct Buffer {
    virtual bool trace(quint16 deviceID, bool enabled = true) = 0;

    virtual bool registerSink(Sink*, Mode) = 0;
    virtual void unregisterSink(Sink*) = 0;

    virtual bool writeFragment(const packet::Header&) = 0;
    virtual bool writeFragment(const packet::Payload&) = 0;

    // Start a new frame by inserting a frame header packet.
    virtual bool startFrame() = 0;

    // Update frame header back pointer and size. Probably called from the clock source?
    virtual bool updateFrame() = 0;

    // Remove the last frame from the buffer.
    // TODO: replace with integration for iterators / std::erase.
    virtual void dropLast() = 0;


    virtual FrameIterator rbegin() const = 0;
    virtual FrameIterator rend() const = 0;
    virtual FrameIterator begin() const = 0;
    virtual FrameIterator end() const = 0;
};
} // namespace trace2;
