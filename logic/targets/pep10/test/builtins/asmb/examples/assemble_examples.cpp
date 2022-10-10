#include "catch.hpp"

#include "asmb/create_driver.hpp"
#include "asmb/ir.hpp"
#include "ex_registry.hpp"
#include "ir/directives.hpp"
#include "ir/macro.hpp"
#include "asmdr/utils/listing.hpp"

TEST_CASE("Parse & pack all sample programs", "[asmb::pep10::parser]") {
  // TODO: Fix when joint assembly / linking works again.
  /*
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  auto ex = registry::instance();
  auto fig_os = ex.find("pep10", 9, "00").value();
  auto text_os = fig_os.elements.at(element_type::kPep);
  auto file_os = std::make_shared<masm::project::source>();
  file_os->name = "os";
  file_os->body = text_os;
  for (const auto &figure : ex.figures()) {
    if (figure.elements.find(element_type::kPep) == figure.elements.end())
      continue;
    if (figure.chapter >= 8)
      continue;
    DYNAMIC_SECTION(fmt::format("Assembling fig {}.{}", figure.chapter, figure.fig)) {
      auto project = masm::project::init_project<uint16_t>();
      for (const auto &macro : ex.macros())
        project->macro_registry->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro);
      auto file_user = std::make_shared<masm::project::source_file>();
      file_user->name = "user";
      file_user->body = figure.elements.at(element_type::kPep);
      auto res = driver->assemble_joint(project, file_os, file_user, masm::project::toolchain_stage::PACK);
      CHECK(res.first);
      // Check that making the listing doesn't crash the program.
      masm::utils::generate_listing(project->image->user);
      if (!res.first)
        std::cout << fmt::format("Figure {}.{}", figure.chapter, figure.fig) << std::endl;
    }
  }
   */
}
