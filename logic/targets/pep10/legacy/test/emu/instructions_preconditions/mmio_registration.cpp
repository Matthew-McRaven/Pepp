#include <cstdint>

#include <catch.hpp>

#include "components/storage/layered.hpp"
#include "components/storage/mmio.hpp"
#include "emu/local_machine.hpp"
#include "isa/pep10.hpp"

// Don't do any program simulation here.
TEST_CASE("Test MMIO registration", "[isa::pep10]") {

  // Test addressing mode i
  SECTION("Test accumulator immediate load.") {
    using storage_t = components::storage::Layered<uint16_t, true>;
    auto layered =
        std::make_shared<storage_t>(10, 0, storage_t::ReadMiss::kDefaultValue, storage_t::WriteMiss::kIgnore);
    auto device_in = std::make_shared<components::storage::Input<uint16_t, true>>(10);
    auto device_out = std::make_shared<components::storage::Output<uint16_t, true>>(20);
    layered->append_storage(1, device_in).value();
    layered->append_storage(2, device_out).value();
    auto machine = std::make_shared<isa::pep10::LocalMachine<true>>(layered);
    machine->register_MMIO_address("NUL0", 0x01);
    REQUIRE(machine->device_address("NUL0").value() == 0x01);
    CHECK(machine->input_device("NUL0").value() == device_in.get());
    CHECK(machine->output_device("NUL0").has_error());

    machine->register_MMIO_address("NUL1", 0x02);
    REQUIRE(machine->device_address("NUL1").value() == 0x02);
    CHECK(machine->input_device("NUL1").has_error());
    CHECK(machine->output_device("NUL1").value() == device_out.get());
  }
}
