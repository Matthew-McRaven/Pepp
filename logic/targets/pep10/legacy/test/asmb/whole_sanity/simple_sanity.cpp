#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"

TEST_CASE("Check whole program sanity", "[asmb::pep10::sanity]") {
  // TODO: Re-implement sanity checking.
  /*
    using namespace asmb::pep10::driver;
    auto driver = make_driver();

    SECTION("No .END") {
        auto project = masm::project::init_project<uint16_t>();
        auto file = std::make_shared<masm::project::source_file>();
        file->name = "os";
        file->body = "ASRA\nNOTX\n";
        auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::WHOLE_PROGRAM_SANITY);
        REQUIRE(!res.first);
    }

    SECTION("Multiple .END") {
        auto project = masm::project::init_project<uint16_t>();
        auto file = std::make_shared<masm::project::source_file>();
        file->name = "os";
        file->body = ".END\nASRA\nNOTX\n";
        auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::WHOLE_PROGRAM_SANITY);
        REQUIRE(!res.first);
    }

    SECTION("User program .BURN") {
        auto project = masm::project::init_project<uint16_t>();
        auto file = std::make_shared<masm::project::source_file>();
        file->name = "os";
        file->body = "ASRA\n.BURN 0xFFFF\n.END\n";
        auto res = driver->assemble_joint(project, file, file, masm::project::toolchain_stage::WHOLE_PROGRAM_SANITY);
        REQUIRE(!res.first);
    }

    SECTION("Operating system no .BURN") {
        auto project = masm::project::init_project<uint16_t>();
        auto file = std::make_shared<masm::project::source_file>();
        file->name = "os";
        file->body = "ASRA\n.END\n";
        auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::WHOLE_PROGRAM_SANITY);
        REQUIRE(!res.first);
    }

    SECTION("Operating system two .BURN") {
        auto project = masm::project::init_project<uint16_t>();
        auto file = std::make_shared<masm::project::source_file>();
        file->name = "os";
        file->body = ".BURN 0xFFFF\nASRA\n.BURN 0xFFFF\n";
        auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::WHOLE_PROGRAM_SANITY);
        REQUIRE(!res.first);
    }
    */
}
