#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ex_registry.hpp"
#include "ir/directives.hpp"
#include "ir/empty.hpp"
#include "asmdr/project/init_project.hpp"
#include "asmdr/project/project.hpp"
#include "asmdr/utils/listing.hpp"

TEST_CASE("Convert to pepb") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();

  // TODO: Fix when bbuiltins work.
  /*auto ex = registry::instance();
  auto fig_os = ex.find("pep10", 9, "00").value();
  auto text_os = fig_os.elements.at(element_type::kPep);
  auto file_os = std::make_shared<masm::project::source>();
  file_os->name = "os";
  file_os->body = text_os;*/
  // TODO: Fix when we have ability to generate listing from linked file
  /*
  SECTION("No .END") {
    for (const auto &macro : ex.macros()) {
      CHECK(project->macro_registry->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro));
    }
    auto file_user = std::make_shared<masm::project::source>();
    file_user->name = "user";
    file_user->body = "ADDA 0xdead,i\nNOTX;comment here\n.END\n";
    auto res = driver->assemble_joint(project, file_os, file_user, masm::project::toolchain_stage::FINISHED);
    REQUIRE(res.first);
    std::string formatted = ::masm::utils::generate_pretty_object_code(project->image->user, 2);
    CHECK(formatted == "0000 1010 0000 1101 1110 1010 1101\n0003 0001 0001                ;comment here\n");

  }*/
}

TEST_CASE("Convert to peph") {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  /* Fix when builtins work.
  auto ex = registry::instance();
  auto fig_os = ex.find("pep10", 9, "00").value();
  auto text_os = fig_os.elements.at(element_type::kPep);
  auto file_os = std::make_shared<masm::project::source>();
  file_os->name = "os";
  file_os->body = text_os;
   */
  // TODO: Fix when we have ability to generate listing from linked file
  /*
  SECTION("Add a single macro") {
    for (const auto &macro : ex.macros()) {
      CHECK(project->macro_registry->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro));
    }
    auto file_user = std::make_shared<masm::project::source>();
    file_user->name = "user";
    file_user->body = "ADDA 0xdead,i\nNOTX;comment here\n.END\n";
    auto res = driver->assemble_joint(project, file_os, file_user, masm::project::toolchain_stage::FINISHED);
    REQUIRE(res.first);
    std::string formatted = ::masm::utils::generate_pretty_object_code(project->image->user, 16);
    CHECK(formatted == "0000 A0DEAD\n0003 11     ;comment here\n");
  }*/
}
