#include <catch.hpp>
#include <zpp_bits.h>
#include "sim/trace2/bytes.hpp"

TEST_CASE("Variable-length spans" "[sim][trace]") {
    auto [data, in, out] = zpp::bits::data_in_out();

    // Figure out  default-size of a packet payload.
    auto start = out.position();
    REQUIRE(out(trace2::Bytes<32>{.len=0}).code == std::errc());
    auto end = out.position();
    REQUIRE(end > start);
    auto size = end-start;

    out.reset();
    start = out.position();
    REQUIRE(out(trace2::Bytes<32>{.len=3}).code == std::errc());
    end = out.position();

    // Increasing the number of bytes in the value should increase the size of the packet.
    CHECK(end > start);
    CHECK((end - start) > size);
    REQUIRE((end - start) - size == 3);

}

int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
}
