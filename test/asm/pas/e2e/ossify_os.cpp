/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include <catch.hpp>
#include "core/resources/figures/book.hpp"
#include "core/resources/figures/builtin_registry.hpp"
#include "help/builtins/figure_wrappers.hpp"
#include "toolchain/macro/declaration.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"

namespace {
void loadBookMacros(std::shared_ptr<const pepp::Book> book, QSharedPointer<macro::Registry> registry) {
  for (auto &macro : book->macros()) {
    // TODO: hideous conversion from current book type to the old macro type. Refactor to remove this copy.
    const auto arch = pepp::arch_as_string(macro->arch);
    auto macroDecl = QSharedPointer<::macro::Declaration>::create(
        QString::fromStdString(macro->name), macro->argcount, QString::fromStdString(macro->body),
        QString::fromStdString(arch), QString::fromStdString(macro->family), macro->hidden);
    registry->registerMacro(::macro::types::Core, macroDecl);
  }
}

} // namespace
TEST_CASE("Avoid breaking changes to CS6E operating system", "[scope:asm][kind:e2e][arch:pep10][scope:lol]") {
  auto fs = builtins::QtFilesystemProvider::create();
  auto bookReg = pepp::BuiltinRegistry(std::move(fs));
  auto book = bookReg.find_book("Computer Systems, 6th Edition");

  REQUIRE(book != nullptr);
  auto registry = QSharedPointer<macro::Registry>::create();
  loadBookMacros(book, registry);
  auto fig = book->find_figure("os", "pep10os");
  auto osBody = QString::fromStdString(fig->default_fragment_text());

  auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(
      {{osBody, {.isOS = true, .ignoreUndefinedSymbols = false}}}, registry);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

  CHECK(pipeline->pipelines.size() == 1);

  auto osTarget = pipeline->pipelines[0].first;
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  if (pipeline->pipelines[0].first->stage != pas::driver::pep10::Stage::End) {
    for (auto &error : pas::ops::generic::collectErrors(*osRoot)) qCritical() << "OS:   " << error.second.message;
    REQUIRE(false);
  }
  REQUIRE(osRoot->has<pas::ast::generic::SymbolTable>());
  const auto symtab = osRoot->get<pas::ast::generic::SymbolTable>().value;
  using P = std::pair<QString, uint16_t>;
  const std::vector<P> items = {{"osRAM", 0xFACA},  {"disp", 0xFB52},    {"trap", 0xFBF7},
                                {"trpHnd", 0xFFF7}, {"initPC", 0xFFF9},  {"initSp", 0xFFFB},
                                {"charIn", 0xFFFD}, {"charOut", 0xFFFE}, {"pwrOff", 0xFFFF}};
  for (const auto &[name, expected] : items) {
    REQUIRE(symtab->exists(name));
    auto sym = symtab->get(name);
    CHECK((sym.value()->value->value().bitPattern & 0xFFFF) == expected);
  }
}
