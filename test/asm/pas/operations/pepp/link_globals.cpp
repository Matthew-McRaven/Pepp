/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "toolchain/pas/operations/generic/link_globals.hpp"
#include <catch.hpp>
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/driver/common.hpp"
#include "toolchain/pas/driver/pepp.hpp"
#include "toolchain/symbol/entry.hpp"
#include "toolchain/symbol/table.hpp"
#include "toolchain/symbol/value.hpp"
#include "bts/isa/pep10.hpp"
using isa::Pep10;
TEST_CASE("Link Globals", "[scope:asm][kind:unit][arch:pep10]") {
  SECTION("Intra-tree link") {
    QString body = "s:.block 10\n.EXPORT s\nLDWA s,i\n.END\n.END";
    auto globals = QSharedPointer<pas::driver::Globals>::create();
    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(body, nullptr);
    REQUIRE(!ret.hadError);
    pas::ops::generic::linkGlobals(*ret.root, globals, {"EXPORT"});
    REQUIRE(globals->contains("s"));
    auto sym = globals->get("s");
    CHECK(sym->binding == symbol::Binding::kGlobal);
  }
  SECTION("Inter-tree link") {
    QString body = "LDWA s,i";
    auto globals = QSharedPointer<pas::driver::Globals>::create();
    auto otherTable = QSharedPointer<symbol::Table>::create(2);
    otherTable->define("s");
    otherTable->markGlobal("s");
    globals->add(*otherTable->get("s"));

    // Verify that global symbol looks correct
    REQUIRE(globals->contains("s"));
    CHECK(globals->get("s")->binding == symbol::Binding::kGlobal);

    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(body, nullptr);
    REQUIRE(!ret.hadError);
    pas::ops::generic::linkGlobals(*ret.root, globals, {"EXPORT"});

    // Verify that local symbol is correct
    REQUIRE(ret.root->has<pas::ast::generic::SymbolTable>());
    auto thisTable = ret.root->get<pas::ast::generic::SymbolTable>().value;
    REQUIRE(thisTable->exists("s"));
    auto thisSym = *thisTable->get("s");
    CHECK(thisSym->binding == symbol::Binding::kImported);
    CHECK(thisSym->state == symbol::DefinitionState::kSingle);
    auto casted = dynamic_cast<symbol::value::ExternalPointer *>(&*thisSym->value);
    CHECK(casted != nullptr);
    CHECK(casted->symbol_pointer == *otherTable->get("s"));
    CHECK(casted->symbol_table == otherTable);
  }
  SECTION("Multi-define") {
    QString body = "s:LDWA s,i";
    auto globals = QSharedPointer<pas::driver::Globals>::create();
    auto otherTable = QSharedPointer<symbol::Table>::create(2);
    otherTable->define("s");
    otherTable->markGlobal("s");
    globals->add(*otherTable->get("s"));

    // Verify that global symbol looks correct
    REQUIRE(globals->contains("s"));
    CHECK(globals->get("s")->binding == symbol::Binding::kGlobal);

    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(body, nullptr);
    REQUIRE(!ret.hadError);
    pas::ops::generic::linkGlobals(*ret.root, globals, {"EXPORT"});

    // Verify that local symbol is correct
    REQUIRE(ret.root->has<pas::ast::generic::SymbolTable>());
    auto thisTable = ret.root->get<pas::ast::generic::SymbolTable>().value;
    REQUIRE(thisTable->exists("s"));
    auto thisSym = *thisTable->get("s");
    CHECK(thisSym->binding == symbol::Binding::kImported);
    CHECK(thisSym->state == symbol::DefinitionState::kExternalMultiple);
  }
}
