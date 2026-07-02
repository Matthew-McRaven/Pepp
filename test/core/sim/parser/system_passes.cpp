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
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

TEST_CASE("System Parser, System, Passes", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  using namespace bits;

  SECTION("with name, compatible") {
    static const char *js = R"j({
      "compatible": "system,root",
      "basename": "/"
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
  }
  SECTION("missing basename") {
    static const char *js = R"j({
      "compatible": "system,root"
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
  }
  SECTION("missing compatible") {
    static const char *js = R"j({
      "basename": "/"
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
  }
  SECTION("Name other than /") {
    static const char *js = R"j({
      "compatible": "system,root",
      "basename": "day"
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/day");
    CHECK(s->config().fullname == "/day");
    CHECK(s->config().compatible == System::compatible);
  }
  SECTION("missing compatible, basename") {
    static const char *js = R"j({
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
  }
  SECTION("ignores extra fields") {
    static const char *js = R"j({
      "beef":"chicken"
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
  }
  SECTION("ignores computed fields") {
    static const char *js = R"j({
      "id":7,
      "fullname": "/feed"
    })j";
    ParsingContext ctx;
    auto s = parse_system(js, ctx);
    REQUIRE(s != nullptr);
    CHECK(s->config().basename == "/");
    CHECK(s->config().fullname == "/");
    CHECK(s->config().compatible == System::compatible);
    CHECK(s->id() == Device::ID{0});
  }
}
