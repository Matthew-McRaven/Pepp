#include <catch.hpp>

#include "sim/device/broadcast/mmo.hpp"

auto desc = sim::api2::device::Descriptor{
    .id = 1, .baseName = "cin", .fullName = "/cin"
};

auto rw = sim::api2::memory::Operation{
  .type = sim::api2::memory::Operation::Type::Standard,
  .kind = sim::api2::memory::Operation::Kind::data,
};
auto app = sim::api2::memory::Operation{
  .type = sim::api2::memory::Operation::Type::Application,
  .kind = sim::api2::memory::Operation::Kind::data,
};
auto span = sim::api2::memory::AddressSpan<quint16>{
  .minOffset = 0, .maxOffset = 0
};

TEST_CASE("Memory-mapped output write, v2", "[sim][memory]") {
    auto out =
        QSharedPointer<sim::memory::Output<quint16>>::create(desc, span, 0);
    auto endpoint = out->endpoint();
    quint8 tmp = 10;
    REQUIRE_NOTHROW(out->write(0, {&tmp, 1}, rw));
    tmp = 20;
    REQUIRE_NOTHROW(out->write(0, {&tmp, 1}, rw));
    auto _1 = endpoint->next_value();
    REQUIRE(_1.has_value());
    CHECK(*_1 == 10);
    auto _2 = endpoint->next_value();
    REQUIRE(_2.has_value());
    CHECK(*_2 == 20);

    // Application writes to MMO are no-ops, therefor no additional values to consume.
    tmp = 30;
    REQUIRE_NOTHROW(out->write(0, {&tmp, 1}, app));
    CHECK(endpoint->at_end());
    auto _3 = endpoint->current_value();
    REQUIRE(_3.has_value());
    CHECK(*_3 == 20);
}

int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
}
