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

#include <catch.hpp>
#include "core/isa/pep/pep10.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/driver/pepp.hpp"
#include "toolchain/pas/operations/pepp/assign_addr.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"

using M = isa::Pep10::Mnemonic;
TEST_CASE("To bytes", "[scope:asm][kind:unit][arch:pep10]") {
  auto [name, source, bytes] = GENERATE(table<std::string, QString, QList<quint8>>(
      {{"BYTE 0xFF", ".BYTE 0xFF", {0xff}},
       {"BYTE 254", ".BYTE 254", {0xfe}},
       {"BYTE -2", ".BYTE -2", {0xfe}},
       {"WORD 0xFFFF", ".WORD 0xFFFF", {0xff, 0xff}},
       {"WORD 65534", ".WORD 65534", {0xff, 0xfe}},
       {"WORD -2", ".WORD -2", {0xff, 0xfe}},
       {"BLOCK 0", ".BLOCK 0", {}},
       {"BLOCK 1", ".BLOCK 1", {0x00}},
       {"BLOCK 2", ".BLOCK 2", {0x00, 0x00}},
       {"BLOCK 3", ".BLOCK 3", {0x00, 0x00, 0x00}},

       {"ASCII short string: no escaped", ".ASCII \"hi\"", {0x68, 0x69}},
       {"ASCII short string: 1 escaped", ".ASCII\".\\n\"", {0x2E, 0x0A}},
       {"ASCII short string: 2 escaped", ".ASCII \"\\0\\n\"", {0x00, 0x0A}},
       {"ASCII short string: 2 hex", ".ASCII \"\\xff\\x00\"", {0xFF, 0x00}},
       {"ASCII long string: no escaped", ".ASCII \"ahi\"", {0x61, 0x68, 0x69}},
       {"ASCII long string: 1 escaped", ".ASCII\".a.\\n\"", {0x2E, 0x61, 0x2E, 0x0A}},
       {"ASCII long string: 2 escaped", ".ASCII\"a\\0\\n\"", {0x61, 0x00, 0x0A}},
       {"ASCII long string: 2 hex", ".ASCII \"a\\xff\\x00\"", {0x61, 0xFF, 0x00}},

       {"ALIGN 1 @ 0", ".ALIGN 1", {}},
       {"ALIGN 2 @ 0", ".ALIGN 2", {}},
       {"ALIGN 2 @ 1", ".BYTE 0xFF\n .ALIGN 2", {0xFF, 0x00}},
       {"ALIGN 4 @ 0", ".ALIGN 4", {}},
       {"ALIGN 4 @ 1", ".BYTE 0xFF\n .ALIGN 4", {0xFF, 0x00, 0x00, 0x00}},
       {"ALIGN 4 @ 2", ".WORD 0xFFAA\n .ALIGN 4", {0xFF, 0xAA, 0x00, 0x00}},
       {"ALIGN 8 @ 0", ".ALIGN 8", {}},
       {"ALIGN 8 @ 1", ".BYTE 0xFF\n .ALIGN 8", {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
       {"ALIGN 8 @ 2", ".WORD 0xFFAA\n .ALIGN 8", {0xFF, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},

       {"unary", "ASRA\nRET", {(uint8_t)M::ASRA, (uint8_t)M::RET}},
       {"nonunary", "s:LDWA 0xFAAD,i\nBR s,i", {(uint8_t)M::LDWA, 0xfa, 0xad, (uint8_t)M::BR, 0x00, 0x00}},
       {".IMPORT s", ".IMPORT s", {}},
       {".EXPORT s", ".EXPORT s", {}},
       {".SCALL s", ".SCALL s", {}},
       {".INPUT s", ".INPUT s", {}},
       {".OUTPUT s", ".OUTPUT s", {}},
       {".END", ".END", {}}

      }));
  DYNAMIC_SECTION(name) {
    auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(source, {.isOS = false});
    auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
    pipelines.pipelines.push_back(pipeline);
    pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
    pipelines.globals->macroRegistry = QSharedPointer<macro::Registry>::create();
    REQUIRE(pipelines.assemble(pas::driver::pep10::Stage::AssignAddresses));
    CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::WholeProgramSanity);
    REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));

    auto root =
        pipelines.pipelines[0].first->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;

    auto actualBytes = pas::ops::pepp::toBytes<isa::Pep10>(*root);
    CHECK(actualBytes == bytes);
  }
}
