#include "asmb_driver.hpp"
#include <utility>
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/langs/asmb_riscv/parser.hpp"

namespace pepp::tc {

DriverResult assemble_riscv(const RISCVDriverConfig &, std::string source) {
  DriverResult result;
  auto rv_parser = parser::RISCVParser(support::SeekableData{std::move(source)});
  auto program = rv_parser.parse(result.diagnostics);
  if (result.diagnostics.count() > 0) return result;

  auto split = riscv_split_to_sections(result.diagnostics, program);
  if (result.diagnostics.count() > 0) return result;

  auto symbols = rv_parser.symbol_table();
  auto addresses = riscv_assign_addresses(split.grouped_ir);
  auto object_code = riscv_to_object_code(addresses, split.grouped_ir);
  result.elf = riscv_to_elf(split.grouped_ir, addresses, object_code);
  write_symbol_table(result.elf, *symbols, object_code);
  return result;
}

} // namespace pepp::tc
