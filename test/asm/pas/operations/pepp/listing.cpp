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
#include "toolchain/pas/driver/pepp.hpp"
#include "toolchain/pas/operations/generic/group.hpp"
#include "toolchain/pas/operations/pepp/addressable.hpp"
#include "toolchain/pas/operations/pepp/assign_addr.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"
#include "enums/isa/pep10.hpp"

TEST_CASE("Format Pepp listing", "[scope:asm][kind:unit][arch:pep10]") {
  auto [name, source, listing] = GENERATE(table<QString, QString, QStringList>({
      {"Blank", "\n", QStringList{""}},
      {"Comment", ";hello\n;world", QStringList{"             ;hello", "             ;world"}},
      {"Unary", "asla\nasra", QStringList{"0000 1A              ASLA", "0001 1C              ASRA"}},
      {"Unary + symbol", "abcdefg:asla", QStringList{"0000 1A     abcdefg: ASLA"}},
      {"Nonunary non-br", "adda 0xfaad,i\nsuba 0xbaad,sfx",
       QStringList{"0000 50FAAD          ADDA     0xFAAD,i", "0003 67BAAD          SUBA     0xBAAD,sfx"}},
      {"Nonunary br", "br 10,i\nbr 20,x",
       QStringList{"0000 24000A          BR       10", "0003 250014          BR       20,x"}},

      {"ALIGN 1", ".ALIGN 1", QStringList{"0000                 .ALIGN   1"}},
      {"ALIGN 2 @ 0", ".ALIGN 2", QStringList{"0000                 .ALIGN   2"}},
      {"ALIGN 2 @ 1", ".BYTE 1\n.ALIGN 2",
       QStringList{"0000 01              .BYTE    1", "0001 00              .ALIGN   2"}},
      {"ALIGN 4 @ 0", ".ALIGN 4", QStringList{"0000                 .ALIGN   4"}},
      {"ALIGN 4 @ 1", ".BYTE 1\n.ALIGN 4",
       QStringList{"0000 01              .BYTE    1", "0001 000000          .ALIGN   4"}},
      {"ALIGN 8 @ 0", ".ALIGN 8", QStringList{"0000                 .ALIGN   8"}},
      {"ALIGN 8 @ 1", ".BYTE 1\n.ALIGN 8",
       QStringList{"0000 01              .BYTE    1", "0001 000000          .ALIGN   8", "     000000", "     00    "}},
      {"ALIGN 8 @ 2", ".WORD 1\n.ALIGN 8",
       QStringList{"0000 0001            .WORD    1", "0002 000000          .ALIGN   8", "     000000"}},

      {"ASCII 2-string", ".ASCII \"hi\"", QStringList{"0000 6869            .ASCII   \"hi\""}},
      {"ASCII 3-string", ".ASCII \"hel\"", QStringList{"0000 68656C          .ASCII   \"hel\""}},
      {"ASCII >3-string", ".ASCII \"hello\"", QStringList{"0000 68656C          .ASCII   \"hello\"", "     6C6F  "}},

      {"BLOCK 0", "s: .BLOCK 0", QStringList{"0000        s:       .BLOCK   0"}},
      {"BLOCK 1", ".BLOCK 1", QStringList{"0000 00              .BLOCK   1"}},
      {"BLOCK 0x2", ".BLOCK 0x2", QStringList{"0000 0000            .BLOCK   0x0002"}},
      {"BLOCK 4", ".BLOCK 4", QStringList{"0000 000000          .BLOCK   4", "     00    "}},

      {"BYTE 0xFE", ".BYTE 0xFE", QStringList{"0000 FE              .BYTE    0xFE"}},

      {"END: no comment", ".END", QStringList{"                     .END"}},
      {"END: comment", ".END;the world", QStringList{"                     .END             ;the world"}},

      {"EQUATE: no comment", "s:.EQUATE 10", QStringList{"            s:       .EQUATE  10"}},
      {"EQUATE: comment", "s:.EQUATE 10;hi", QStringList{"            s:       .EQUATE  10      ;hi"}},
      {"EQUATE: symbolic", "s:.EQUATE y\n.block 0x3\ny:.block 0",
       QStringList{"            s:       .EQUATE  y", "0000 000000          .BLOCK   0x0003",
                   "0003        y:       .BLOCK   0"}},
      {"EQUATE: hex", "s:.EQUATE 0x2", QStringList{"            s:       .EQUATE  0x0002"}},
      {"EQUATE: unsigned", "s:.EQUATE 2", QStringList{"            s:       .EQUATE  2"}},
      {"EQUATE: signed", "s: .EQUATE -2", QStringList{"            s:       .EQUATE  -2"}},
      {"EQUATE: char", "s: .EQUATE 's'", QStringList{"            s:       .EQUATE  's'"}},
      {"EQUATE: string", "s: .EQUATE \"hi\"", QStringList{"            s:       .EQUATE  \"hi\""}},

      {"ORG", ".BLOCK 1\n.ORG 0x8000\n.BLOCK 1",
       QStringList{"0000 00              .BLOCK   1", "                     .ORG     0x8000",
                   "8000 00              .BLOCK   1"}},

      {"WORD 0xFFFE", ".WORD 0xFFFE", QStringList{"0000 FFFE            .WORD    0xFFFE"}},

      {".IMPORT", ".IMPORT s", QStringList{"                     .IMPORT  s"}},
      {".EXPORT", ".EXPORT s", QStringList{"                     .EXPORT  s"}},
      {".SCALL", ".SCALL s", QStringList{"                     .SCALL   s"}},
      {".INPUT", ".INPUT s", QStringList{"                     .INPUT   s"}},
      {".OUTPUT", ".OUTPUT s", QStringList{"                     .OUTPUT  s"}},

  }));
  DYNAMIC_SECTION("") {
    auto parsed = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(source, nullptr);
    auto str = parsed.errors.join("\n").toStdString();
    REQUIRE_FALSE(parsed.hadError);
    pas::ops::generic::groupSections(*parsed.root, pas::ops::pepp::isAddressable<isa::Pep10>);
    pas::ops::pepp::assignAddresses<isa::Pep10>(*parsed.root);
    auto actualListing = pas::ops::pepp::formatListing<isa::Pep10>(*parsed.root, {.bytesPerLine = 3});
    REQUIRE(actualListing.size() == listing.size());
    auto actualListingText = actualListing.join("\n").toStdString();
    CHECK(actualListingText == listing.join("\n").toStdString());
  }
}
