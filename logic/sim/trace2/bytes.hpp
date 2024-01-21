#pragma once
#include <qtypes.h>
#include <zpp_bits.h>

namespace trace2 {
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
} // namespace trace2
