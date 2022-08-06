#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ex_registry.hpp"
#include "ir/directives.hpp"
#include "ir/macro.hpp"

TEST_CASE("Check that reported byte length matches actual byte length.", "[asmb::pep10::addr_assign]") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  SECTION("Check OS length.") {

    auto ex = registry();
    auto maybe_cs6e = ex.find_book("Computer Systems, 6th Edition");
    REQUIRE(maybe_cs6e);
    auto cs6e = *maybe_cs6e;
    /* TODO: Fix when builtins are stable.
    auto fig_os = ex.find("pep10", 9, "00").value();
    auto text_os = fig_os.elements.at(element_type::kPep);*/
    auto file = std::make_shared<masm::project::source>();
    file->name = "os";
    file->body = ""; //text_os;
    for (const auto &macro : cs6e->macros())
      CHECK(file->macro_registry->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro));
    // TODO: Fix test when assembler driver is fixed.
    /*auto res = driver->assemble_os(project, file, masm::project::toolchain_stage::PACK);

    REQUIRE(res.first);
    for (const auto &line : project->image->os->body_ir->ir_lines) {
        std::vector<uint8_t> bytes;
        line->append_object_code(bytes);
        if (line->emits_object_code)
            CHECK(line->object_code_bytes() == bytes.size());
        if (line->emits_object_code && (line->object_code_bytes() != bytes.size())) {
            std::cout << "Damn";
        }
    }*/
  }
}
