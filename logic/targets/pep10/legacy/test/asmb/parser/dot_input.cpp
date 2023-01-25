#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/directives.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot input", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Valid .INPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = "s:asra\n.INPUT s\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 2);
    auto maybe_input = section->ir_lines[1];
    auto as_input = std::dynamic_pointer_cast<ir::dot_input<uint16_t>>(maybe_input);
    REQUIRE(as_input);

    // Check for externalized symbol definition.
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    // TODO: Check that "s" is marked as global
    // TODO: Check that there is only one external symbol.
    // TODO: Check that the list of external symbols starts with a symbol named "s".
    // auto externs = symbol::externals<uint16_t>(project->image->symbol_table->entries());
    // REQUIRE(boost::size(externs) == 1);
    // CHECK((*externs.begin())->name == "s");
  }

  SECTION("Valid .INPUT + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".INPUT s ;Hi guys\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_input = section->ir_lines[0];
    auto as_input = std::dynamic_pointer_cast<ir::dot_input<uint16_t>>(maybe_input);
    REQUIRE(as_input);
    REQUIRE(as_input->comment);
    CHECK(as_input->comment.value() == "Hi guys");

    // Check for externalized symbol definition.
    REQUIRE(symbol::exists<uint16_t>(section->symbol_table, "s"));
    // TODO: Check that "s" is marked as global
  }

  SECTION("Invalid .INPUT + symbol + comment") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    // TODO: Ban self-references on input, since input generates no object code.
    file->body = "s: .INPUT s ;Hi guys\n"; // Self reference is actually okay here, but has no use.
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No dec in .INPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".INPUT 22\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No hex in .INPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".INPUT 0xbeef\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No signed dec in .input") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".input -19\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }

  SECTION("No string in .INPUT") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".INPUT \"HI\"\n";
    file->target_type = ir::section::section_type::kOperatingSystem;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE_FALSE(res.first);
  }
}
