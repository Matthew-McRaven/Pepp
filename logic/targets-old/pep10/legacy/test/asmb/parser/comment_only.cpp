#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"

TEST_CASE("Parse comment line", "[asmb::pep10::parser]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Comment line") {
    auto file = std::make_shared<masm::project::source>();
    file->name = "main";
    file->body = ";Hello world!\n";
    file->target_type = ir::section::section_type::kUserProgram;

    auto group = masm::project::init_group<uint16_t>({file});
    auto res = driver->assemble(group, masm::project::toolchain_stage::SYMANTIC);
    REQUIRE(res.first);
    REQUIRE(group->targets.size() == 1);
    auto target = group->targets[0];
    REQUIRE(target->container->sections.size() == 1);
    auto section = target->container->sections[0];
    REQUIRE(section->ir_lines.size() == 1);
    auto maybe_comment = section->ir_lines[0];
    auto as_comment = std::dynamic_pointer_cast<ir::comment_line<uint16_t>>(maybe_comment);
    REQUIRE(as_comment);
  }
}
