#include <catch.hpp>

#include "sim/device/broadcast/mmi.hpp"

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
auto span = sim::api::memory::AddressSpan<quint16>{
    .minOffset = 0, .maxOffset = 0
};

TEST_CASE("Memory-mapped input read, v2", "[sim][memory]") {
    auto in =
        QSharedPointer<sim::memory::Input<quint16>>::create(desc, span, 0);
    auto endpoint = in->endpoint();
    endpoint->append_value(10);
    endpoint->append_value(20);
    quint8 tmp;
    // Read advances state
    REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, rw));
    CHECK(tmp ==10);
    // Get does not modify current value.
    REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, app));
    CHECK(tmp == 10);
    // Read advances state
    REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, rw));
    CHECK(tmp == 20);

    // Soft-fail MMI, should yield default value
    in->setFailPolicy(sim::api::memory::FailPolicy::YieldDefaultValue);
    REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, rw));
    CHECK(tmp == 0);
    // Hard-fail MMI should throw
    in->setFailPolicy(sim::api::memory::FailPolicy::RaiseError);
    REQUIRE_THROWS_AS(in->read(0, {&tmp, 1}, rw), sim::api2::memory::Error<quint16>);
}

int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
}
