#include "catch.hpp"
#include <elfio/elfio.hpp>

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "elf/mem_map.hpp"
#include "ex_registry.hpp"
#include "ir/directives.hpp"
#include "ir/macro.hpp"
#include "emu/from_elf.hpp"
#include "ir/section/section.hpp"
#include "utils/format.hpp"

/*
std::shared_ptr<masm::elf::AnnotatedImage<uint16_t>> joint_to_image(const std::string &user_prog) {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  auto project = masm::project::init_project<uint16_t>();
  auto ex = registry::instance();
  for (const auto &macro : ex.macros()) {
    CHECK(project->macro_registry->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro));
  }

  auto fig_os = ex.find("pep10", 9, "00").value();
  auto text_os = fig_os.elements.at(element_type::kPep);
  auto file_os = std::make_shared<masm::project::source_file>();
  file_os->name = "os";
  file_os->body = text_os;

  auto file_user = std::make_shared<masm::project::source_file>();
  file_user->name = "user";
  file_user->body = user_prog;

  auto res = driver->assemble_joint(project, file_os, file_user, masm::project::toolchain_stage::FINISHED);
  std::cout << res.second;
  REQUIRE(res.first);
  REQUIRE(project->as_elf);
  return project->as_elf;
}*/

TEST_CASE("Pep/10 ELF image line<=>address mapping", "[masm::elf::pep10]") {

  // TODO: Restore when listing is back in.
  SECTION("address=>line mapping in user program.") {
    /*auto image = joint_to_image(".WORD 52\n.WORD28\n.END");
    // Word spans two addresses
    auto line_0 = image->listing_line_from_address(0);
    CHECK(line_0);
    CHECK(*line_0 == 0);
    line_0 = image->listing_line_from_address(1);
    CHECK(line_0);
    CHECK(*line_0 == 0);

    // Word spans two addresses
    auto line_1 = image->listing_line_from_address(2);
    CHECK(line_1);
    CHECK(*line_1 == 1);
    line_1 = image->listing_line_from_address(3);
    CHECK(line_1);
    CHECK(*line_1 == 1);*/
  }
  /*SECTION("address=>line mapping in user program.") {
      auto image = joint_to_image(".WORD 52\n.WORD28\n.END");

      auto line_0 = image->address_from_listing_line(0);
      CHECK(line_0);
      CHECK(*line_0 == 0);

      // Word spans two addresses
      auto line_1 = image->address_from_listing_line(1);
      CHECK(line_1);
      CHECK(*line_1 == 2);
  }*/
}
