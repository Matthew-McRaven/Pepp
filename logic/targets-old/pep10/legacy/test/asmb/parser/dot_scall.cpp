#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/directives.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot SCALL", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Valid .SCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:asra\n.SCALL s\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_SCALL = section->ir_lines[1];
    auto as_SCALL = std::dynamic_pointer_cast<asmb::pep10::dot_scall<uint16_t>>(maybe_SCALL);
    REQUIRE(as_SCALL);

    // Check for externalized symbol definition.
    // REQUIRE(project->image->symbol_table->exists("s"));
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    REQUIRE(target->macro_registry->contains("s"));
  }

  SECTION("Valid .SCALL + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".SCALL s ;Hi guys\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_SCALL = section->ir_lines[0];
    auto as_SCALL = std::dynamic_pointer_cast<asmb::pep10::dot_scall<uint16_t>>(maybe_SCALL);
    REQUIRE(as_SCALL);
    REQUIRE(as_SCALL->comment);
    CHECK(as_SCALL->comment.value() == "Hi guys");

    // Check for externalized symbol definition.
    // REQUIRE(project->image->symbol_table->exists("s"));
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    REQUIRE(target->macro_registry->contains("s"));
  }

  SECTION("Invalid .SCALL + symbol + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    // TODO: Ban self-references on.SCALL, since.SCALL generates no object code.
    file->body = "s: .SCALL s ;Hi guys\n"; // Self reference is actually okay here, but has no use.
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No dec in .SCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".SCALL 22\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No hex in .SCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".SCALL 0xbeef\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No signed dec in .SCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".SCALL -19\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No string in .SCALL") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".SCALL \"HI\"\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
