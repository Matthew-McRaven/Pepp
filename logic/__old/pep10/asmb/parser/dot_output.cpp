#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/directives.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot output", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Valid .OUTPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:asra\n.OUTPUT s\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_output = section->ir_lines[1];
    auto as_output = std::dynamic_pointer_cast<ir::dot_output<uint16_t>>(maybe_output);
    REQUIRE(as_output);

    // Check for externalized symbol definition.
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    // TODO: Check that "s" is marked as global
    // TODO: Check that there is only one external symbol.
    // TODO: Check that the list of external symbols starts with a symbol named "s".
    // auto externs = symbol::externals<uint16_t>(project->image->symbol_table->entries());
    // REQUIRE(boost::size(externs) == 1);
    // CHECK((*externs.begin())->name == "s");
  }

  SECTION("Valid .OUTPUT + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".OUTPUT s ;Hi guys\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_output = section->ir_lines[0];
    auto as_output = std::dynamic_pointer_cast<ir::dot_output<uint16_t>>(maybe_output);
    REQUIRE(as_output);
    REQUIRE(as_output->comment);
    CHECK(as_output->comment.value() == "Hi guys");

    // Check for externalized symbol definition.
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    // TODO: Check that "s" is marked as global
  }

  SECTION("Invalid .OUTPUT + symbol + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    // TODO: Ban self-references on output, since output generates no object code.
    file->body = "s: .OUTPUT s ;Hi guys\n"; // Self reference is actually okay here, but has no use.
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No dec in .OUTPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".OUTPUT 22\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No hex in .OUTPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".OUTPUT 0xbeef\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No signed dec in .output") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".output -19\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No string in .OUTPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".OUTPUT \"HI\"\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
