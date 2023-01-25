#include "catch.hpp"
#include <elfio/elfio.hpp>

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "elf/mem_map.hpp"
#include "ex_registry.hpp"
#include "ir/directives.hpp"
#include "ir/macro.hpp"
#include "emu/from_elf.hpp"
#include "ir/section/section.hpp"
#include "utils/format.hpp"

std::shared_ptr<masm::elf::AnnotatedImage<uint16_t>> os_to_image() {
  // TODO: Fill in when linker works
  /*
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  auto project = masm::project::init_project<uint16_t>();
  auto ex = registry::instance();
  for (const auto &macro : ex.macros()) {
    CHECK(project->macro_registry->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro));
  }
  auto fig_os = ex.find("pep10", 9, "00").value();
  auto text_os = fig_os.elements.at(element_type::kPep);
  auto file_os = std::make_shared<masm::project::source_file>();
  file_os->name = "os";
  file_os->body = text_os;
  auto res = driver->assemble_os(project, file_os, masm::project::toolchain_stage::FINISHED);
  std::cout << res.second;
  REQUIRE(res.first);
  REQUIRE(project->as_elf);
  return project->as_elf;*/
  return nullptr;
}

std::shared_ptr<masm::elf::AnnotatedImage<uint16_t>> joint_to_image(const std::string &user_prog) {
  // TODO: fill in when linker works...
  /*
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  auto project = masm::project::init_project<uint16_t>();
  auto ex = registry::instance();
  for (const auto &macro : ex.macros()) {
    CHECK(project->macro_registry->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro));
  }

  auto fig_os = ex.find("pep10", 9, "00").value();
  auto text_os = fig_os.elements.at(element_type::kPep);
  auto file_os = std::make_shared<masm::project::source_file>();
  file_os->name = "os";
  file_os->body = text_os;

  auto file_user = std::make_shared<masm::project::source_file>();
  file_user->name = "user";
  file_user->body = user_prog;

  auto res = driver->assemble_joint(project, file_os, file_user, masm::project::toolchain_stage::FINISHED);
  std::cout << res.second;
  REQUIRE(res.first);
  REQUIRE(project->as_elf);
  return project->as_elf;*/
  return nullptr;
}

TEST_CASE("Convert ELF image to Pep/10 machine", "[elf_tools::pep10]") {
  // TOOD: Fix when linker works...
  /*
  SECTION("Assemble OS successfully.") {
    REQUIRE(os_to_image() != nullptr);
  }

  SECTION("ROM in correct location and is correct size.") {
    auto image = os_to_image();
    auto ROM_result = elf_tools::pep10::construct_os_storage < false > (image->image());
    REQUIRE(ROM_result.has_value());
    auto [ROM_offset, ROM_storage] = ROM_result.value();
    // OS ROM should not unexpectedly change in size.
    CHECK(ROM_offset == 0xFAAc);
    CHECK(ROM_storage->max_offset() == 0x553);
    // And the default OS should go right up to the maximum offset.
    CHECK(ROM_offset + ROM_storage->max_offset() == 0xFFFF);
  }

  SECTION("RAM in correct location and is correct size.") {
    auto image = os_to_image();
    auto RAM_result = elf_tools::pep10::construct_ram < false > (image->image());
    REQUIRE(RAM_result.has_value());
    auto [RAM_offset, RAM_storage] = RAM_result.value();
    // RAM should not unexpectedly change in size or location.
    CHECK(RAM_offset == 0x0000);
    CHECK(RAM_storage->max_offset() == 0xFAAB);
  }

  SECTION("MMIO definitions are correct.") {
    auto image = os_to_image();
    auto ports_result = elf_tools::pep10::port_definitions(image->image());
    REQUIRE(ports_result.has_value());
    auto ports = ports_result.value();

    // Pep/10 should have 4 MMIO ports with the given address/type properties.
    CHECK(ports.size() == 4);
    for (auto &port : ports) {
      if (port.name == "charIn") {
        CHECK(port.type == masm::elf::mmio::Type::kInput);
        CHECK(port.offset == 0xFAA9);
      } else if (port.name == "charOut") {
        CHECK(port.type == masm::elf::mmio::Type::kOutput);
        CHECK(port.offset == 0xFAAA);
      } else if (port.name == "diskIn") {
        CHECK(port.type == masm::elf::mmio::Type::kInput);
        CHECK(port.offset == 0xFAA8);
      } else if (port.name == "pwrOff") {
        CHECK(port.type == masm::elf::mmio::Type::kOutput);
        CHECK(port.offset == 0xFAAB);
      } else {
        CHECK((false && "Bad MMIO definition"));
      }
    }
  }

  SECTION("MMIO ports in correct location and is correct count.") {
    auto image = os_to_image();
    auto ports_result = elf_tools::pep10::construct_mmio_ports < false > (image->image());
    REQUIRE(ports_result.has_value());
    auto ports = ports_result.value();

    // Pep/10 should have 4 MMIO ports with the given address/type properties.
    CHECK(ports.size() == 4);
    for (auto &[offset, storage] : ports) {
      if (offset == 0xFAA9) { // charIn
        auto ptr = std::dynamic_pointer_cast<components::storage::Input<uint16_t, false, uint8_t>>(storage);
        CHECK(ptr != nullptr);
      } else if (offset == 0xFAAA) { // charOut
        auto ptr = std::dynamic_pointer_cast<components::storage::Output<uint16_t, false, uint8_t>>(storage);
        CHECK(ptr != nullptr);
      } else if (offset == 0xFAA8) { // diskIn
        auto ptr = std::dynamic_pointer_cast<components::storage::Input<uint16_t, false, uint8_t>>(storage);
        CHECK(ptr != nullptr);
      } else if (offset == 0xFAAB) { // pwrOff
        auto ptr = std::dynamic_pointer_cast<components::storage::Output<uint16_t, false, uint8_t>>(storage);
        CHECK(ptr != nullptr);
      } else {
        CHECK((false && "Not a valid MMIO port."));
      }
    }
  }

  SECTION("Layered memory device has correct layout.") {
    auto image = os_to_image();
    auto storage_result = elf_tools::pep10::construct_memory_map < false > (image->image());
    REQUIRE(storage_result.has_value());
    auto storage = storage_result.value();
    // Pick two addresses that are definitely RAM/ROM, so that we can check that the correct device is returned.
    auto RAM_baseline = storage->device_at(0x0000).value();
    auto ROM_baseline = storage->device_at(0xFFFF).value();

    // Test that RAM spans from 0x0000 to 0xFAA8
    for (uint16_t it = 0; it < 0xFAA8; it++) {
      auto RAM_result = storage->device_at(it);
      REQUIRE(RAM_result.has_value());
      auto RAM = dynamic_cast<components::storage::Block<uint16_t, false, uint8_t> *>(RAM_result.value());
      CHECK(RAM != nullptr);
      CHECK(RAM == RAM_baseline);
    }

    // Test that ROM spans from 0xFAAC to 0xFFFF
    for (uint32_t it = 0xFAAC; it <= 0xFFFF; it++) {
      auto ROM_result = storage->device_at((uint16_t) it);
      REQUIRE(ROM_result.has_value());
      auto ROM = dynamic_cast<components::storage::Block<uint16_t, false, uint8_t> *>(ROM_result.value());
      CHECK(ROM != nullptr);
      CHECK(ROM == ROM_baseline);
    }

    // Test MMIO ports in alphabetic order
    auto charIn_result = storage->device_at(0xFAA9);
    REQUIRE(charIn_result.has_value());
    auto charIn = dynamic_cast<components::storage::Input<uint16_t, false, uint8_t> *>(charIn_result.value());
    CHECK(charIn != nullptr);

    auto charOut_result = storage->device_at(0xFAAA);
    REQUIRE(charOut_result.has_value());
    auto charOut = dynamic_cast<components::storage::Output<uint16_t, false, uint8_t> *>(charOut_result.value());
    CHECK(charOut != nullptr);

    auto diskIn_result = storage->device_at(0xFAA8);
    REQUIRE(diskIn_result.has_value());
    auto diskIn = dynamic_cast<components::storage::Input<uint16_t, false, uint8_t> *>(diskIn_result.value());
    CHECK(diskIn != nullptr);

    auto pwrOff_result = storage->device_at(0xFAAB);
    REQUIRE(pwrOff_result.has_value());
    auto pwrOff = dynamic_cast<components::storage::Output<uint16_t, false, uint8_t> *>(pwrOff_result.value());
    CHECK(pwrOff != nullptr);
  }

  SECTION("Validate machine creation.") {
    auto image = os_to_image();
    auto machine_result = isa::pep10::machine_from_elf<false>(image->image());
    REQUIRE(machine_result.has_value());
    auto machine = machine_result.value();

    // Test MMIO ports in alphabetic order
    auto charIn_lookup = machine->input_device("charIn");
    CHECK(charIn_lookup.has_value());

    auto charOut_lookup = machine->output_device("charOut");
    CHECK(charOut_lookup.has_value());

    auto diskIn_lookup = machine->input_device("diskIn");
    CHECK(diskIn_lookup.has_value());

    auto pwrOff_lookup = machine->output_device("pwrOff");
    CHECK(pwrOff_lookup.has_value());
  }

  SECTION("Buffer user program to diskIn.") {
    auto image = joint_to_image(".WORD 52\n.WORD28\n.END");
    auto machine_result = isa::pep10::machine_from_elf<false>(image->image());
    REQUIRE(machine_result.has_value());
    auto machine = machine_result.value();
    REQUIRE(isa::pep10::load_user_program(image->image(), machine, isa::pep10::Loader::kDiskIn));

    auto diskIn_lookup = machine->input_device("diskIn");
    CHECK(diskIn_lookup.has_value());
    auto diskIn = diskIn_lookup.value();

    auto bytes_result = elf_tools::section_as_bytes(image->image(), "user.text");
    CHECK(bytes_result.has_value());
    auto formatted_bytes = utils::bytes_to_hex_string(bytes_result.value(), 16, true);
    auto bytes = std::vector<uint8_t>(formatted_bytes.begin(), formatted_bytes.end());
    CHECK(bytes.size() == 14);

    for (int it = 0; it < bytes.size(); it++) {
      CHECK(diskIn->read(0x0).value() == bytes[it]);
    }
  }

  SECTION("Buffer user program to RAM.") {
    auto image = joint_to_image(".WORD 52\n.WORD28\n.END");
    auto machine_result = isa::pep10::machine_from_elf<false>(image->image());
    REQUIRE(machine_result.has_value());
    auto machine = machine_result.value();
    REQUIRE(isa::pep10::load_user_program(image->image(), machine, isa::pep10::Loader::kRAM));

    auto bytes_result = elf_tools::section_as_bytes(image->image(), "user.text");
    CHECK(bytes_result.has_value());
    auto bytes = bytes_result.value();
    CHECK(bytes.size() == 4);

    for (int it = 0; it < bytes.size(); it++) {
      CHECK(machine->read_memory(it).value() == bytes[it]);
    }
  }
   */
}
