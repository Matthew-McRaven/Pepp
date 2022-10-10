#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/directives.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot USCALL", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Valid .USCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:asra\n.USCALL s\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_USCALL = section->ir_lines[1];
    auto as_USCALL = std::dynamic_pointer_cast<asmb::pep10::dot_uscall<uint16_t>>(maybe_USCALL);
    REQUIRE(as_USCALL);

    // Check for externalized symbol definition.
    // REQUIRE(project->image->symbol_table->exists("s"));
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    REQUIRE(target->macro_registry->contains("s"));
  }

  SECTION("Valid .USCALL + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".USCALL s ;Hi guys\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_USCALL = section->ir_lines[0];
    auto as_USCALL = std::dynamic_pointer_cast<asmb::pep10::dot_uscall<uint16_t>>(maybe_USCALL);
    REQUIRE(as_USCALL);
    REQUIRE(as_USCALL->comment);
    CHECK(as_USCALL->comment.value() == "Hi guys");

    // Check for externalized symbol definition.
    // REQUIRE(project->image->symbol_table->exists("s"));
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    REQUIRE(target->macro_registry->contains("s"));
  }

  SECTION("Invalid .USCALL + symbol + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    // TODO: Ban self-references on.USCALL, since.USCALL generates no object code.
    file->body = "s: .USCALL s ;Hi guys\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No dec in .USCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".USCALL 22\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No hex in .USCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".USCALL 0xbeef\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No signed dec in .USCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".USCALL -19\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No string in .USCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".USCALL \"HI\"\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
