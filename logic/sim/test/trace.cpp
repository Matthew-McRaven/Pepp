#include "qtypes.h"
#include <catch.hpp>
#include <zpp_bits.h>

namespace packet {

// Stack-allocated variable length array of bytes. When serialized, it will only occupy
// the number of bytes specified in data_len, not the size as specified by N.
// One use for this class is to serialize addresses, which may be 1-8 bytes long.
// These variable-length addresses allow us to use the same packet for all memory accesses.
// For example, in Ppe/10 a register is a 1 byte address, where main memory is 2.
// Our old trace system needed a packet type for each address size.
// The variable-length byte-array allows this to be expressed in a single packet type.
template <size_t N>
struct Bytes {
    // TODO: add helper methods to convert to/from  un/signed int8/16/32/64 in either endianness.
    constexpr static zpp::bits::errc serialize(auto& archive, auto& self) {
        // Serialize array manually to avoid allocating extra 0's in the bit stream.
        // Must also de-serialize manually, otherwise archive will advance the position
        // by the allocated size of the array, not the "used" size.
        if(archive.kind() == zpp::bits::kind::out) {

            if(self.len > N) return zpp::bits::errc(std::errc::value_too_large);
            zpp::bits::errc errc = archive(self.len);
            if(errc.code != std::errc()) return errc;
            else if(self.len == 0) return errc;

            // We serialized the length ourselves. If we pass array_view directly, size will be serialzed again.
            auto array_view = std::span<quint8>(self.bytes.data(), self.len);
            return archive(zpp::bits::bytes(array_view, array_view.size_bytes()));
        } else {
            zpp::bits::errc errc = archive(self.len);
            if(errc.code != std::errc()) return errc;
            else if(self.len > N) return zpp::bits::errc(std::errc::value_too_large);
            else if(self.len == 0) return errc;

            auto array_view = std::span<quint8>(self.bytes.data(), self.len);
            // See above.
            return archive(zpp::bits::bytes(array_view, array_view.size_bytes()));
        }
    }
    quint8 len = 0;
    std::array<quint8, N> bytes = {0};
};

using device_id_t = zpp::bits::varint<quint16>;

struct Header{
    quint16 length = 0;
    zpp::bits::varint<quint16> back_offset = 0;
};


namespace clock {
struct Tick{
    device_id_t clock = 0;
};
} // namespace clock

namespace io {

}

// I am choosing to forgo inheritance here.
// I want to have control over the memory layout of each access type,
// Depending on the in the dervied classes, we may have pointless padding added.
namespace memory {
struct Clear {
    device_id_t device = 0;
    packet::Bytes<8> value = {0,0};
};

struct PureRead {
    device_id_t device = 0;
    zpp::bits::varint<quint64> payload_len = 0;
    packet::Bytes<8> address = {0,0};
};

// Packet MUST be followed by a "Payload" packet.
struct ImpureRead {
    device_id_t device = 0;
    packet::Bytes<8> address = {0,0};
};

// Packet MUST be followed by a "Payload" packet.
struct Write {
    device_id_t device = 0;
    packet::Bytes<8> address = {0,0};
};

// Successive payloads belong to the same read/write transaction.
struct Payload {
    packet::Bytes<32> payload = {0, 0};
};

} // namespace memory

} // namespace packet

//
TEST_CASE("Variable-length spans" "[sim][trace]") {
    auto [data, in, out] = zpp::bits::data_in_out();

    // Figure out  default-size of a packet payload.
    auto start = out.position();
    REQUIRE(out(packet::Bytes<32>{.len=0}).code == std::errc());
    auto end = out.position();
    REQUIRE(end > start);
    auto size = end-start;

    out.reset();
    start = out.position();
    REQUIRE(out(packet::Bytes<32>{.len=3}).code == std::errc());
    end = out.position();

    // Increasing the number of bytes in the value should increase the size of the packet.
    CHECK(end > start);
    CHECK((end - start) > size);
    REQUIRE((end - start) - size == 3);

}
TEST_CASE("Basic trace tests", "[sim][trace]") {
    std::variant<packet::Header, packet::memory::PureRead, packet::memory::Write> packet;
}

int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
}
