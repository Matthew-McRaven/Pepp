/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "core/compile/lex/buffer.hpp"
#include <catch.hpp>
#include <memory>
#include "core/compile/lex/tokens.hpp"
#include "core/langs/asmb_pep/lexer.hpp"

TEST_CASE("Token buffer", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  using namespace pepp::tc::lex;
  auto pool = std::make_shared<std::unordered_set<std::string>>();
  PepLexer lexer(pool, pepp::tc::support::SeekableData{std::string{"abc def\nghi"}});
  Buffer buf(&lexer);

  SECTION("Unbuffering without checkpoints") {
    REQUIRE(buf.match<Identifier>());
    REQUIRE(buf.peek<Identifier>()); // Lexes def without matching it.
    buf.unbuffer();
    CHECK(buf.buffered_tokens().empty());
    CHECK(lexer.cursor().rest() == " def\nghi");
    REQUIRE(buf.match<Identifier>()->to_string() == "def");
    // Across a newline.
    REQUIRE(buf.peek<Empty>());
    buf.unbuffer();
    CHECK(lexer.cursor().rest() == "\nghi");
    REQUIRE(buf.match<Empty>());
    CHECK(buf.match<Identifier>()->to_string() == "ghi");
  }
}
