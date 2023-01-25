#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot burn", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("no decimal .BURN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BURN 33\n";
    file->target_type = ir::section::section_type::kOperatingSystem;
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("no signed decimal .BURN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BURN -33\n";
    file->target_type = ir::section::section_type::kOperatingSystem;
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

    // TODO:
  SECTION("hex .BURN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BURN 0x21\n";
    file->target_type = ir::section::section_type::kOperatingSystem;
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto os_target = group->targets[0];
    auto os_sections = os_target->container->sections;
    REQUIRE(os_sections.size() == 1);
    auto os_section = os_sections[0];
    REQUIRE(os_section->ir_lines.size() == 1);
    auto maybe_burn = os_section->ir_lines[0];
    auto as_burn = std::dynamic_pointer_cast<ir::dot_burn<uint16_t>>(maybe_burn);
    REQUIRE(as_burn->argument->value() == 33);
  }

  SECTION("char .BURN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BURN '!'\n";
    file->target_type = ir::section::section_type::kOperatingSystem;
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("no string .BURN") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".BURN \"!\"\n";
    file->target_type = ir::section::section_type::kOperatingSystem;
    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
