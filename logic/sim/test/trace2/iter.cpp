#include <catch.hpp>
#include <zpp_bits.h>
#include "sim/api2.hpp"
#include "sim/trace2/buffers.hpp"
#include "sim/trace2/packet_utils.hpp"

TEST_CASE("Trace buffer iterators" "[sim][trace]") {
    sim::trace2::InfiniteBuffer buf;
    std::array<quint8, 16> src, dest;
    for(int it=0; it< src.size(); it++) {
        src[it] = 0xFE;
        dest[it] = it;
    }
    buf.trace(1, true);
    buf.trace(2, true);
    sim::trace2::emitFrameStart(&buf);
    sim::trace2::emitWrite<quint16>(&buf, 1, 0, src, dest);
    sim::trace2::emitWrite<quint16>(&buf, 1, 32, src, dest);
    sim::trace2::emitWrite<quint16>(&buf, 1, 64, src, dest);
    sim::trace2::emitPureRead<quint16>(&buf, 1, 0, 16);
    sim::trace2::emitFrameStart(&buf);
    buf.updateFrameHeader();

    // FORWARD ITERATION
    {
        // Two frames.
        CHECK(std::distance(buf.cbegin(), buf.cend()) == 2);

        // Four packets.
        auto frame = buf.cbegin();
        CHECK(std::distance(frame.cbegin(), frame.cbegin()) == 0);
        CHECK(std::distance(frame.cend(), frame.cend()) == 0);
        CHECK(std::distance(frame.cbegin(), frame.cend()) == 4);

        auto packet = frame.cbegin();
        for (int it = 0; it < 3; it++) {
            // One payload.
            sim::api2::packet::Header header = *packet;
            CHECK(std::holds_alternative<sim::api2::packet::header::Write>(header));
            auto wr = std::get<sim::api2::packet::header::Write>(header);
            CHECK(wr.address.to_address<quint16>() == 32 * it);
            CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
            CHECK(std::distance(packet.cend(), packet.cend()) == 0);
            CHECK(std::distance(packet.cbegin(), packet.cend()) == 1);
            ++packet;
        }

        // Pure read, no payloads
        CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
        CHECK(std::distance(packet.cend(), packet.cend()) == 0);
        CHECK(std::distance(packet.cbegin(), packet.cend()) == 0);
        ++packet;

        CHECK(packet == frame.cend());
    }
    // REVERSE ITERATION
    // Two frames.
    {
        auto b = buf.crbegin();
        auto e = buf.crend();
        CHECK(std::distance(buf.crbegin(), buf.crend()) == 2);
        // Starts with 0 packets.
        auto frame = buf.crbegin();
        // Check forwards and backwards iterators.
        CHECK(std::distance(frame.cbegin(), frame.cbegin()) == 0);
        CHECK(std::distance(frame.cend(), frame.cend()) == 0);
        CHECK(std::distance(frame.cbegin(), frame.cend()) == 0);
        CHECK(std::distance(frame.crbegin(), frame.crbegin()) == 0);
        CHECK(std::distance(frame.crend(), frame.crend()) == 0);
        CHECK(std::distance(frame.crbegin(), frame.crend()) == 0);
        ++frame;

        // Four packets.
        // Check forwards and backwards iterators.
        CHECK(std::distance(frame.cbegin(), frame.cbegin()) == 0);
        CHECK(std::distance(frame.cend(), frame.cend()) == 0);
        CHECK(std::distance(frame.cbegin(), frame.cend()) == 4);
        CHECK(std::distance(frame.crbegin(), frame.crbegin()) == 0);
        CHECK(std::distance(frame.crend(), frame.crend()) == 0);
        CHECK(std::distance(frame.crbegin(), frame.crend()) == 4);
        // Forward iteration within a frame
        {
            auto packet = frame.crbegin();
            for (int it = 0; it < 3; it++) {
                // One payload.
                sim::api2::packet::Header header = *packet;
                CHECK(std::holds_alternative<sim::api2::packet::header::Write>(header));
                auto wr = std::get<sim::api2::packet::header::Write>(header);
                CHECK(wr.address.to_address<quint16>() == 32 * it);
                CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
                CHECK(std::distance(packet.cend(), packet.cend()) == 0);
                CHECK(std::distance(packet.cbegin(), packet.cend()) == 1);
                CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
                CHECK(std::distance(packet.crend(), packet.crend()) == 0);
                CHECK(std::distance(packet.crbegin(), packet.crend()) == 1);
                ++packet;
            }

            // Pure read, no payloads
            CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
            CHECK(std::distance(packet.cend(), packet.cend()) == 0);
            CHECK(std::distance(packet.cbegin(), packet.cend()) == 0);
            CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
            CHECK(std::distance(packet.crend(), packet.crend()) == 0);
            CHECK(std::distance(packet.crbegin(), packet.crend()) == 0);
            ++packet;

            CHECK(packet == frame.crend());
        }
        // Reverse iteration within a frame
        {
            auto packet = frame.cbegin();
            // Pure read, no payloads
            CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
            CHECK(std::distance(packet.cend(), packet.cend()) == 0);
            CHECK(std::distance(packet.cbegin(), packet.cend()) == 0);
            CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
            CHECK(std::distance(packet.crend(), packet.crend()) == 0);
            CHECK(std::distance(packet.crbegin(), packet.crend()) == 0);
            ++packet;

            for (int it = 0; it < 3; it++) {
                // One payload.
                sim::api2::packet::Header header = *packet;
                CHECK(std::holds_alternative<sim::api2::packet::header::Write>(header));
                auto wr = std::get<sim::api2::packet::header::Write>(header);
                CHECK(wr.address.to_address<quint16>() == 32 * (2 - it));
                CHECK(std::distance(packet.cbegin(), packet.cbegin()) == 0);
                CHECK(std::distance(packet.cend(), packet.cend()) == 0);
                CHECK(std::distance(packet.cbegin(), packet.cend()) == 1);
                CHECK(std::distance(packet.crbegin(), packet.crbegin()) == 0);
                CHECK(std::distance(packet.crend(), packet.crend()) == 0);
                CHECK(std::distance(packet.crbegin(), packet.crend()) == 1);
                ++packet;
            }

            CHECK(packet == frame.cend());
        }
    }
}

int main(int argc, char* argv[]) {
    return Catch::Session().run( argc, argv );
}
