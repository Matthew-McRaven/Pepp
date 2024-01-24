#include <catch.hpp>

#include "sim/api2.hpp"
#include "sim/device/dense.hpp"
#include "sim/device/simple_bus_v2.hpp"

auto rw = sim::api2::memory::Operation {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

auto d1 = sim::api2::device::Descriptor{
                                       .id = 1, .baseName = "d1", .fullName = "/bus0/d1"};
auto d2 = sim::api2::device::Descriptor{
                                       .id = 2, .baseName = "d2", .fullName = "/bus0/d2"};
auto d3 = sim::api2::device::Descriptor{
                                       .id = 3, .baseName = "d3", .fullName = "/bus0/d3"};
auto b1 = sim::api2::device::Descriptor{
                                       .id = 4, .baseName = "bus0", .fullName = "/bus0"};
using Span = sim::api2::memory::AddressSpan<quint16>;
auto make = []() {
    auto m1 = QSharedPointer<sim::memory::Dense<quint16>>::create(
        d1, Span{.minOffset = 0, .maxOffset = 0x1});
    auto m2 = QSharedPointer<sim::memory::Dense<quint16>>::create(
        d2, Span{.minOffset = 0, .maxOffset = 0x1});
    auto m3 = QSharedPointer<sim::memory::Dense<quint16>>::create(
        d3, Span{.minOffset = 0, .maxOffset = 0x1});
    auto bus = QSharedPointer<sim::memory::SimpleBus2<quint16>>::create(
        b1, Span{.minOffset = 0, .maxOffset = 5});
    bus->pushFrontTarget(Span{.minOffset = 0, .maxOffset = 1}, &*m1);
    bus->pushFrontTarget(Span{.minOffset = 2, .maxOffset = 3}, &*m2);
    bus->pushFrontTarget(Span{.minOffset = 4, .maxOffset = 5}, &*m3);
    return std::tuple{bus, m1, m2, m3};
};

TEST_CASE("Simple bus individual in-bounds access, v2", "[sim][memory][throws]") {
    auto [bus, m1, m2, m3] = make();
    sim::api2::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
    quint8 buf[2];
    bits::span bufSpan = {buf};
    bits::memclr(bufSpan);

    // Can write to each individual memory and read on bus.
    for (int i = 0; i < 2; i++) {
        auto m = memArr[i];
        bits::memcpy_endian(bufSpan, bits::Order::BigEndian, quint16(0x0001));
        REQUIRE_NOTHROW(m->write(0, bufSpan, rw));
        bits::memclr(bufSpan);
        REQUIRE_NOTHROW(bus->read(0 + i * 2, bufSpan, rw));
        for (int j = 0; j < 1; j++)
            CHECK(buf[j] == j);
    }
}

TEST_CASE("Simple bus group in-bounds access, v2", "[sim][memory][throws]") {
    auto [bus, m1, m2, m3] = make();
    sim::api2::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
    quint8 buf[6];
    bits::span bufSpan = {buf};
    for (int it = 0; it < 6; it++)
        buf[it] = it;
    REQUIRE_NOTHROW(bus->write(0, {buf}, rw));
    bits::memclr(bufSpan);

    // Can write to bus and read each individual memory.
    for (int i = 0; i < 2; i++) {
        auto m = memArr[i];
        REQUIRE_NOTHROW(m->read(0, bufSpan.first(2), rw));
        for (int j = 0; j < 1; j++)
            CHECK(buf[j] == i * 2 + j);
    }
}

TEST_CASE("Simple bus dump, v2", "[sim][memory]") {
    SECTION("dense memory map") {
        auto [bus, m1, m2, m3] = make();
        sim::api2::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
        quint8 buf[6];
        bits::span bufSpan = {buf};
        for (int it = 0; it < 6; it++)
            buf[it] = it;
        REQUIRE_NOTHROW(bus->write(0, {buf}, rw));
        bits::memclr(bufSpan);

        bus->dump({buf});

        for (int it = 0; it < 6; it++)
            CHECK(buf[it] == it);
    }
    SECTION("sparse memory map") {
        auto m1 = QSharedPointer<sim::memory::Dense<quint16>>::create(
            d1, Span{.minOffset = 0, .maxOffset = 0x1});
        auto m2 = QSharedPointer<sim::memory::Dense<quint16>>::create(
            d2, Span{.minOffset = 0, .maxOffset = 0x1});
        auto m3 = QSharedPointer<sim::memory::Dense<quint16>>::create(
            d3, Span{.minOffset = 0, .maxOffset = 0x1});
        auto bus = QSharedPointer<sim::memory::SimpleBus2<quint16>>::create(
            b1, Span{.minOffset = 0, .maxOffset = 9});
        bus->pushFrontTarget(Span{.minOffset = 0, .maxOffset = 1}, &*m1);
        bus->pushFrontTarget(Span{.minOffset = 4, .maxOffset = 5}, &*m2);
        bus->pushFrontTarget(Span{.minOffset = 8, .maxOffset = 9}, &*m3);
        quint8 buf[10], out[10];
        bits::span bufSpan = {buf};
        bits::span outSpan = {out};
        bits::memclr(bufSpan);
        bits::memclr(outSpan);
        bits::memcpy_endian(bufSpan.subspan(0, 2), bits::Order::BigEndian,
                            quint16(0x0001));
        m1->write(0, bufSpan.subspan(0, 2), rw);
        bits::memcpy_endian(bufSpan.subspan(4, 2), bits::Order::BigEndian,
                            quint16(0x0405));
        m2->write(0, bufSpan.subspan(4, 2), rw);
        bits::memcpy_endian(bufSpan.subspan(8, 2), bits::Order::BigEndian,
                            quint16(0x0809));
        m3->write(0, bufSpan.subspan(8, 2), rw);

        bus->dump({out});
        for (int it = 0; it < sizeof(out); it++)
            CHECK(buf[it] == out[it]);
    }
}


int main( int argc, char* argv[] ) {
    return Catch::Session().run( argc, argv );
}
