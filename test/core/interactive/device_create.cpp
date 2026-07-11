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
#include <catch.hpp>
#include "core/interactive_test/hostobjs/vocab.hpp"
#include "core/interactive_test/interp.hpp"
#include "core/interactive_test/vocab/core_words.hpp"

TEST_CASE("REPL -- simulator & device mangament", "[scope:core][scope:core.repl][kind:unit][arch:*]") {

  SECTION("Create a dense RAM device") {
    const std::string input = "cfg.alloc root sys.alloc\n"
                              "cfg.alloc ram,dense\n"
                              "cfg.set min_offset 0\n"
                              "cfg.set max_offset 0xfeed\n"
                              "cfg.set basename test\n"
                              "dev.alloc dev.id .\n"
                              "dev.fullname\n"
                              ". print0\n";
    Interpreter p;
    register_common_words(&p);
    register_devicemgmt_words(&p);
    p.output = std::make_unique<BufferedOutput>();
    auto out = static_cast<BufferedOutput *>(p.output.get());
    p.run_on(input);
    CHECK(out->buffer.size() == 3);
    CHECK(out->buffer[0].starts_with("1"));
    CHECK(out->buffer[1].starts_with("5"));
    CHECK(out->buffer[2].starts_with("/test"));
  }
}
