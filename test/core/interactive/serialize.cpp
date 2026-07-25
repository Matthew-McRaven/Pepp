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
#include <nlohmann/json.hpp>
#include "core/interactive_test/hostobjs/vocab.hpp"
#include "core/interactive_test/interp.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "fmt/ranges.h"

TEST_CASE("REPL -- serialization of devices", "[scope:core][scope:core.repl][kind:unit][arch:*]") {

  SECTION("Create and serialize a dense RAM device") {
    const std::string input = "cfg.alloc root sys.alloc\n"
                              "cfg.alloc ram,dense\n"
                              "cfg.set min_offset 0\n"
                              "cfg.set max_offset 0xfeed\n"
                              "cfg.set basename test\n"
                              "dev.alloc";
    Interpreter p;
    register_common_words(&p);
    register_devicemgmt_words(&p);
    p.run_on(input);
    p.output = std::make_unique<BufferedOutput>();
    auto out = static_cast<BufferedOutput *>(p.output.get());
    p.run_on("sys.json");
    auto line = fmt::format("{}", fmt::join(out->buffer.begin(), out->buffer.end(), ""));
    auto actual_result = nlohmann::json::parse(line);
    static const auto text = R"({
    "basename": "/",
    "children": [
      {
        "basename": "test",
        "compatible": "ram,dense",
        "max_offset": 65261,
        "min_offset": 0
      }
    ],
    "compatible": "system,root"
  })";
    auto expected_result = nlohmann::json::parse(text);
    CHECK(actual_result == expected_result);
  }
}
