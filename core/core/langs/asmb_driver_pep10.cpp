#include "asmb_driver.hpp"
#include <memory>
#include <utility>
#include "core/compile/macro/macro_registry.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/parser.hpp"

namespace pepp::tc {

DriverResult assemble_pep10(const Pep10DriverConfig &, std::string source) {
  DriverResult result;
  auto macros = std::make_shared<MacroRegistry>();
  auto pep_parser = parser::PepParser(support::SeekableData{std::move(source)}, macros);
  auto program = pep_parser.parse(result.diagnostics);
  if (result.diagnostics.count() > 0) return result;

  auto flattened = parser::flatten_macros(program);
  auto split = pepp_split_to_sections(result.diagnostics, flattened);
  if (result.diagnostics.count() > 0) return result;

  auto symbols = pep_parser.symbol_table();
  auto addresses = pepp_assign_addresses(split.grouped_ir);
  auto object_code = pepp_to_object_code(addresses, split.grouped_ir);
  result.elf = pepp_to_elf(split.grouped_ir, addresses, object_code, split.mmios);
  write_symbol_table(result.elf, *symbols, object_code);
  return result;
}

} // namespace pepp::tc
