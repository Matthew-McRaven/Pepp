/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QRegularExpression>
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "core/langs/asmb_driver.hpp"
#include "core/resources/figures/book.hpp"
#include "core/resources/figures/builtin_registry.hpp"
#include "help/builtins/figure_wrappers.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/macro/registry.hpp"

// Compares the new assembler's bytes against pas's, per address, for the CS5E figures and OS.
namespace {
static const auto lf = QRegularExpression("\r");
using ByteMap = std::map<u32, u8>;
using Symdefs = std::vector<std::pair<std::string, u32>>;
// Without a linker, user programs see the OS's MMIO ports as constants from the CS5E memory map.
const Symdefs os_ports = {{"charIn", 0xFC15}, {"charOut", 0xFC16}};

QString text_of(const pepp::Figure &figure) {
  return QString::fromStdString(figure.find_fragment("pep")->contents()).replace(lf, "");
}

// pas's bytes are contiguous from the program's lowest address.
ByteMap bytes_of_pas(const QList<quint8> &bytes, u32 offset) {
  ByteMap ret;
  for (qsizetype it = 0; it < bytes.size(); it++) ret[offset + it] = bytes[it];
  return ret;
}

// Assemble with assemble_pep9, then read every allocated section's bytes at its address.
ByteMap bytes_of_core(const QString &source, const Symdefs &symdefs) {
  auto result = pepp::tc::assemble_pep9({.symdefs = symdefs}, {}, source.toStdString());
  for (const auto &[_, diagnostic] : result.diagnostics) UNSCOPED_INFO(diagnostic.message);
  REQUIRE(result.ok());
  std::stringstream stream;
  pepp::tc::write_elf(result.elf, stream);
  ELFIO::elfio elf;
  REQUIRE(elf.load(stream));
  ByteMap ret;
  for (const auto &sec : elf.sections)
    if (sec->get_type() == ELFIO::SHT_PROGBITS && (sec->get_flags() & ELFIO::SHF_ALLOC))
      for (ELFIO::Elf_Xword it = 0; it < sec->get_size(); it++)
        ret[sec->get_address() + it] = static_cast<u8>(sec->get_data()[it]);
  return ret;
}
} // namespace

TEST_CASE("Pep/9 new assembler matches pas", "[scope:core][scope:core.langs][kind:e2e][arch:pep9]") {
  auto bookReg = pepp::BuiltinRegistry(builtins::QtFilesystemProvider::create());
  auto book = bookReg.find_book("Computer Systems, 5th Edition");
  REQUIRE(book != nullptr);
  // CS5E has no macros.
  auto reg = QSharedPointer<macro::Registry>::create();

  SECTION("CS5E OS") {
    const pepp::Figure *os = nullptr;
    for (auto &figure : book->figures())
      if (figure->is_os() && figure->has_fragment("pep")) os = &*figure;
    REQUIRE(os != nullptr);
    helpers::AsmHelper helper(reg, text_of(*os), pepp::Architecture::PEP9);
    REQUIRE(helper.assemble());
    const auto bytes = helper.bytes(true);
    // The OS ends in .BURN 0xFFFF, so its last byte is at 0xFFFF.
    CHECK(bytes_of_pas(bytes, 0x1'0000 - bytes.size()) == bytes_of_core(text_of(*os), {}));
  }

  for (auto &figure : book->figures()) {
    if (figure->is_os() || !figure->has_fragment("pep") || !figure->default_os()) continue;
    DYNAMIC_SECTION("Figure " << figure->name_chapter() << "." << figure->name_figure()) {
      helpers::AsmHelper helper(reg, text_of(*figure->default_os()), pepp::Architecture::PEP9);
      helper.setUserText(text_of(*figure));
      REQUIRE(helper.assemble());
      CHECK(bytes_of_pas(helper.bytes(false), 0) == bytes_of_core(text_of(*figure), os_ports));
    }
  }
}
