#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include "asmb_driver.hpp"
#include "core/arch/pepx/asmb/pep10.hpp"
#include "core/arch/pepx/asmb/pep9.hpp"
#include "core/compile/macro/macro_registry.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/asmb/listing.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "core/langs/asmb_pep/text_format.hpp"

namespace pepp::tc {
namespace {
// The pipeline shared by every Pep architecture.
DriverResult assemble_pep(const pep::Dialect &dialect, bts::ElfMachineType machine,
                          const std::vector<std::pair<std::string, u32>> &symdefs,
                          std::shared_ptr<MacroRegistry> macros, const DebugLineFiles &debug_files,
                          const FormattingConfig &fmtcfg, std::string source) {
  DriverResult result;
  if (!macros) macros = std::make_shared<MacroRegistry>();
  auto pep_parser = parser::PepParser(support::SeekableData{std::move(source)}, macros, dialect);
  auto sy = pep_parser.symbol_table();
  inject_symdefs(symdefs, *sy, 2);

  auto program = pep_parser.parse(result.diagnostics);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;
  if (fmtcfg.source_format) {
    auto formatted = format_source(dialect, program);
    fmtcfg.source_format(std::move(formatted));
  }

  auto flattened = parser::flatten_macros(program, fmtcfg.listing_format != nullptr);
  auto split = pepp_split_to_sections(result.diagnostics, flattened);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;

  auto addresses = pepp_assign_addresses(result.diagnostics, split.grouped_ir);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;
  auto object_code = pepp_to_object_code(result.diagnostics, addresses, split.grouped_ir);
  if (result.diagnostics.count(Severity::Fatal) > 0) return result;
  std::optional<ListingLayout> layout;
  if (fmtcfg.listing_format) {
    const auto format = [&](const LinearIR &line) { return format_source(dialect, &line); };
    layout = layout_listing(flattened, addresses, pep_listing_bytes_per_line, format);
  }
  // After flattening, so that a definition inside an expansion has its instantiation's line.
  result.set_elf(pepp_to_elf(split.grouped_ir, addresses, object_code, *pep_parser.symbol_table(), split.mmios,
                             debug_files, parser::defined_macros(program), machine, layout ? &layout->rows : nullptr));
  if (layout) {
    auto listing = format_listing(to_input_elf(*result.unchecked_elf()), layout->text, pep_listing_style);
    // The ELF was written from this layout, so a mismatch is a bug in the assembler.
    if (!listing) throw std::logic_error("listing: " + listing.error());
    fmtcfg.listing_format(std::move(*listing));
  }
  return result;
}
} // namespace

DriverResult assemble_pep10(const Pep10DriverConfig &cfg, const FormattingConfig &fmtcfg, std::string source) {
  return assemble_pep(pep::pep10::dialect(), bts::ElfMachineType::EM_PEP10, cfg.symdefs, cfg.macros, cfg.debug_files,
                      fmtcfg, std::move(source));
}

DriverResult assemble_pep9(const Pep9DriverConfig &cfg, const FormattingConfig &fmtcfg, std::string source) {
  return assemble_pep(pep::pep9::dialect(), bts::ElfMachineType::EM_PEP9, cfg.symdefs, nullptr, cfg.debug_files, fmtcfg,
                      std::move(source));
}

} // namespace pepp::tc
