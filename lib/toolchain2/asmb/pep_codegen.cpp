#include "./pep_codegen.hpp"
#include <elfio/elfio.hpp>
#include "fmt/format.h"
#include "toolchain/symbol/table.hpp"
#include "toolchain/symbol/visit.hpp"

std::vector<std::pair<pepp::tc::SectionDescriptor, pepp::tc::PepIRProgram>>
pepp::tc::split_to_sections(PepIRProgram &prog, SectionDescriptor initial_section) {
  using Pair = std::pair<pepp::tc::SectionDescriptor, pepp::tc::PepIRProgram>;
  std::vector<Pair> ret;
  ret.emplace_back(std::make_pair(initial_section, pepp::tc::PepIRProgram{}));
  auto *active = &ret[0];
  for (auto &line : prog) {
    // TODO: Check all symbol usages are not undefined
    // TODO: .BURN for this section.

    // If no existing section has the same name, create a new section with the provided flags.
    // When the section already exists, ensure that the flags match before switching to that section,
    if (auto as_section = std::dynamic_pointer_cast<pepp::tc::ir::DotSection>(line); as_section) {
      auto flags = as_section->flags;
      auto name = as_section->name.to_string();
      auto existing_sec = std::find_if(ret.begin(), ret.end(), [&name](auto &i) { return i.first.name == name; });
      if (existing_sec == ret.end()) {
        pepp::tc::SectionDescriptor desc{.name = name.toStdString(), .flags = flags};
        ret.emplace_back(std::make_pair(desc, pepp::tc::PepIRProgram{}));
        active = &ret.back();
      } else if (existing_sec->first.flags != flags) {
        throw std::logic_error("Modifying flags for an existing section");
      } else active = &*existing_sec;
    } else if (auto as_align = std::dynamic_pointer_cast<pepp::tc::ir::DotAlign>(line); as_align) {
      active->first.alignment = std::max(active->first.alignment, as_align->argument.value->value<quint16>());
    }

    if (auto symbol_attr = line->typed_attribute<ir::attr::SymbolDeclaration>(); symbol_attr) {
      if (!symbol_attr->entry->is_singly_defined()) {
        auto formatted = fmt::format("Multiply defined symbol {}", symbol_attr->entry->name.toStdString());
        throw std::logic_error(formatted);
      }
    }

    active->second.emplace_back(line);
  }
  return ret;
}

pepp::tc::IRMemoryAddressTable
pepp::tc::assign_addresses(std::vector<std::pair<SectionDescriptor, PepIRProgram>> &prog) {
  enum class Direction { Forward, Backward } direction = Direction::Forward;

  // Pre-allocate vector according to the total size of the IR lines in all sections.
  // This overrserves storage---not all IR lines generate object code---but is a stable upper bound and avoid
  // reallocation in the address-assignment loop.
  size_t size = 0;
  for (const auto &sec : prog) size += sec.second.size();

  // ITEMS ARE NOT INSERTED IN SORTED ORDER. DO NOT USE AS A MAP UNTIL SORTING.
  // Since all IR lines across all PepIRProgram have unique (C++) addresses, we can blindly append and sort later.
  // This gives O(1) insert rather than  n* O(nlgn) with the requirement for a manual sort before returning.
  IRMemoryAddressTable ret;
  ret.container.reserve(size);

  for (auto &sec : prog) {
    quint16 base_address = sec.first.base_address.value_or(0);
    for (auto &line : sec.second) {
      quint16 symbol_base = base_address, next_base = base_address, size = line->object_size(base_address).value_or(0);
      if (auto maybe_size = line->object_size(base_address); !maybe_size.has_value()) continue;
      else if (auto as_org = std::dynamic_pointer_cast<ir::DotOrg>(line); as_org) {
        base_address = as_org->argument.value->value<quint16>();
        symbol_base = next_base = base_address;
      } else if (auto as_equate = std::dynamic_pointer_cast<ir::DotEquate>(line); as_equate) {

        auto symbol = as_equate->symbol.entry;
        auto argument = as_equate->argument.value;
        // Re-use from previous assembler
        if (auto symbolic = dynamic_cast<pas::ast::value::Symbolic *>(&*argument); symbolic != nullptr) {
          auto other = symbolic->symbol();
          if (symbol::rootTable(other->parent.sharedFromThis()) == symbol::rootTable(symbol->parent.sharedFromThis())) {
            symbol->value = QSharedPointer<symbol::value::InternalPointer>::create(sizeof(quint16), other);
          } else {
            symbol->value = QSharedPointer<symbol::value::ExternalPointer>::create(
                sizeof(quint16), other->parent.sharedFromThis(), other);
          }
        } else {
          auto bits = symbol::value::MaskedBits{.byteCount = 2, .bitPattern = 0, .mask = 0xFFFF};
          argument->value(bits::span<quint8>{reinterpret_cast<quint8 *>(&bits.bitPattern), 8}, bits::hostOrder());
          symbol->value = QSharedPointer<symbol::value::Constant>::create(bits);
        }
        continue; // Must exit loop early, or symbol will be clobbered below.
      } else if (direction == Direction::Forward) {
        // Must explicitly handle address wrap-around, because math inside set
        // address widens implicitly.
        next_base = (base_address + size) % 0x10000;
        // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
        // in which case no adjustment is necessary.
        ret.container.emplace_back(line.get(), ir::attr::Address(base_address, size));
        base_address = next_base;
      } else {
        next_base = (base_address - size) % 0x10000;
        // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
        // in which case no adjustment is necessary.
        auto adjustedAddress = next_base + (size > 0 ? 1 : 0);
        // If we use newBase, we are off-by-one when size is non-zero.
        symbol_base = adjustedAddress;
        ret.container.emplace_back(line.get(), ir::attr::Address(adjustedAddress % 0x10000, size));
        base_address = next_base;
      }

      if (auto line_symbol = line->typed_attribute<ir::attr::SymbolDeclaration>(); line_symbol) {
        auto isCode = dynamic_cast<ir::DyadicInstruction *>(&*line) || dynamic_cast<ir::MonadicInstruction *>(&*line);
        line_symbol->entry->value = QSharedPointer<symbol::value::Location>::create(
            size, sizeof(quint16), symbol_base, 0, isCode ? symbol::Type::kCode : symbol::Type::kObject);
      }
    }
  }
  // Establish flat_map invariant, which is that the container is sorted.
  std::sort(ret.container.begin(), ret.container.end(), detail::IRComparator{});
  return ret;
}

// Register system calls
// Gather IOs
