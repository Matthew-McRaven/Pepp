#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"

TEST_CASE("Parse dot end", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Lonely End.") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ".END\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_end = section->ir_lines[0];
    auto as_end = std::dynamic_pointer_cast<ir::dot_end<uint16_t>>(maybe_end);
    REQUIRE(as_end);
  }
}
