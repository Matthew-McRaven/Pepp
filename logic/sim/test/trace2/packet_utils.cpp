#include <catch.hpp>
#include <zpp_bits.h>
#include "sim/api2.hpp"
#include "sim/trace2/packet_utils.hpp"
using namespace sim::api2;
using namespace sim::trace2;
using wrapped  = std::variant<frame::Header, packet::Header, packet::Payload>;
using w = wrapped;
struct SimpleBuffer : public sim::api2::trace::Buffer {
    SimpleBuffer(): _data(), _in(_data), _out(_data){}
    // Buffer interface
    bool trace(sim::api2::device::ID deviceID, bool enabled) override {return true;}
    bool registerSink(sim::api2::trace::Sink *, sim::api2::trace::Mode) override {return true;}
    void unregisterSink(sim::api2::trace::Sink *) override {}
    bool writeFragment(const sim::api2::frame::Header & hdr) override {_out(w{hdr}).or_throw(); return true;}
    bool writeFragment(const sim::api2::packet::Header & hdr) override {_out(w{hdr}).or_throw(); return true;}
    bool writeFragment(const sim::api2::packet::Payload & hdr) override {_out(w{hdr}).or_throw(); return true;}
    bool updateFrameHeader() override {return true;}
    void dropLast() override {throw std::logic_error("Unimplemented");}
    TraceIterator cbegin() const override {throw std::logic_error("Unimplemented");}
    TraceIterator cend() const override {throw std::logic_error("Unimplemented");}
    TraceIterator crbegin() const override {throw std::logic_error("Unimplemented");}
    TraceIterator crend() const override {throw std::logic_error("Unimplemented");}

    mutable std::vector<std::byte> _data = {};
    zpp::bits::in<decltype(_data)> _in;
    zpp::bits::out<decltype(_data)> _out;
};

TEST_CASE("Packet IsSameDevice", "[sim][trace]")
{
    using namespace sim::api2::packet;
    auto equal = IsSameDevice{5};
    auto nequal = IsSameDevice{6};
    packet::Header hdr = packet::header::Write{.device = 5, .address = 0};
    CHECK(std::visit(equal, hdr));
    CHECK_FALSE(std::visit(nequal, hdr));
}

TEST_CASE("Packet serialization utilities", "[sim][trace]") {
    SECTION("Pure Read") {
        auto [address, device, payload_len] = GENERATE(table<quint16, device::ID, quint16>({
            {0, 1, 0},
            {0, 1, 1},
            {10, 1, 16},
            // Can emit read with size > VariableBytes::N
            {20, 1, 33},
        }));
        SimpleBuffer buf;
        emitPureRead(&buf, device, address, payload_len);
        wrapped w;
        REQUIRE_NOTHROW(buf._in(w).or_throw());
        REQUIRE(std::holds_alternative<packet::Header>(w));
        auto hdr = std::get<packet::Header>(w);
        REQUIRE(std::holds_alternative<packet::header::PureRead>(hdr));
        auto read = std::get<packet::header::PureRead>(hdr);
        CHECK(read.address.to_address<decltype(address)>() == address);
        CHECK(read.device == device);
        CHECK(read.payload_len == payload_len);
    }

    // kind == 0 => MMRead
    // kind == 1 => MMWrtie
    // kind == 2 => Write
    static const std::string name[] = {"Memory-mapped read", "Memory-mapped write", "Write"};
    for(int kind = 0; kind < 3; kind++) {
        DYNAMIC_SECTION(name[kind]) {
            auto [address, device, payload_len] = GENERATE(table<quint16, device::ID, quint16>({
                // Emit 1 payload frament
                {0, 1, 0},
                {0, 1, 1},
                {10, 1, 16},
                // Emits 2 payload fragments
                {20, 1, 33},
            }));

            static const quint8 source_value = 0xFE;
            std::vector<quint8> src(payload_len), dest(payload_len), expected(payload_len);
            for(int it = 0; it < payload_len; it++) {
                dest[it] = source_value;
                src[it] = it;
                if(kind == 2) expected[it] = it ^ source_value;
                else expected[it] = it;
            }

            SimpleBuffer buf;

            if(kind == 0) emitMMRead(&buf, device, address, {src});
            else if(kind == 1) emitMMWrite(&buf, device, address, {src});
            else emitWrite(&buf, device, address, {src}, {dest});

            wrapped w;
            REQUIRE_NOTHROW(buf._in(w).or_throw());
            REQUIRE(std::holds_alternative<packet::Header>(w));
            auto hdr = std::get<packet::Header>(w);

            // Decode packet header based on if it is read or write.
            if(kind == 0) {
                REQUIRE(std::holds_alternative<packet::header::ImpureRead>(hdr));
                auto read = std::get<packet::header::ImpureRead>(hdr);
                CHECK(read.address.to_address<decltype(address)>() == address);
                CHECK(read.device == device);
            } else if (kind == 1 || kind == 2) {
                REQUIRE(std::holds_alternative<packet::header::Write>(hdr));
                auto read = std::get<packet::header::Write>(hdr);
                CHECK(read.address.to_address<decltype(address)>() == address);
                CHECK(read.device == device);
            }

            int current = 0;
            // Iterate over all payload fragments.
            while(payload_len - current > 0) {
                // Read in payload
                REQUIRE_NOTHROW(buf._in(w).or_throw());
                REQUIRE(std::holds_alternative<packet::Payload>(w));
                auto pay = std::get<packet::Payload>(w);
                REQUIRE(std::holds_alternative<packet::payload::Variable>(pay));
                auto bytes = std::get<packet::payload::Variable>(pay);

                // Determine if there should be further payloads based on input size.
                auto masked_len = bytes.payload.len & bytes.payload.len_mask();
                bool continues = payload_len - current > packet::payload::Variable::N;
                auto expected_len = std::min(packet::payload::Variable::N, (std::size_t)payload_len - current);

                CHECK(bytes.payload.continues() == continues);
                CHECK(masked_len == expected_len);
                if(memcmp(bytes.payload.bytes.data(), expected.data() + current, expected_len) != 0) {
                    int x = 123;
                }
                CHECK(memcmp(bytes.payload.bytes.data(), expected.data() + current, expected_len) == 0);
                current += bytes.payload.len;
            }
        }
    }
}

int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
}
