#include "packet_utils.hpp"

// Max payload size is a compile time constant, so compute at compile time.
using vb = sim::api2::packet::payload::Variable;
static constexpr auto payload_max_size = vb::N;

void sim::trace2::detail::emit_payloads(sim::api2::trace::Buffer *tb,
                                             bits::span<const quint8> buf1,
                                             bits::span<const quint8> buf2) {
    auto data_len = std::min(buf1.size(), buf2.size());
    // Split the data into chunks that are `payload_max_size` bytes long.
    for(int it = 0; it < data_len;) {
        auto payload_len = std::min(data_len - it, payload_max_size);
        bool continues = data_len - it > payload_max_size;
        // Additional payloads needed if it is more than N elements away from data_len.
        auto bytes = api2::packet::VariableBytes<payload_max_size>(payload_len, continues);

        // XOR-encode data to reduce storage by 2x.
        bits::memcpy_xor(bits::span<quint8>{bytes.bytes},
                         buf1.subspan(it, payload_len),
                         buf2.subspan(it, payload_len));

        api2::packet::payload::Variable payload {std::move(bytes)};
        tb->writeFragment({payload});
        it += payload_len;
    }
}

void sim::trace2::detail::emit_payloads(sim::api2::trace::Buffer *tb,
                                        bits::span<const quint8> buf)
{
    auto data_len = buf.size();
    // Split the data into chunks that are `payload_max_size` bytes long.
    for(int it = 0; it < data_len;) {
        auto payload_len = std::min(data_len - it, payload_max_size);
        bool continues = data_len - it > payload_max_size;
        // Additional payloads needed if it is more than N elements away from data_len.
        auto bytes = api2::packet::VariableBytes<payload_max_size>(payload_len, continues);

        bits::memcpy(bits::span<quint8>{bytes.bytes}, buf.subspan(it, payload_len));
        api2::packet::payload::Variable payload {std::move(bytes)};
        tb->writeFragment({payload});
        it += payload_len;
    }
}
