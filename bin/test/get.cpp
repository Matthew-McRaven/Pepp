/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#include "config.hpp"

TEST_CASE("Terminal, get", "[term][cli]") {
  auto path = term_path();
  SECTION("valid pepp source") {
    QProcess term;
    term.start(path, {"-e", "5", "get", "--ch", "05", "--fig", "27", "--type", "pep"});
    wait_return(term, 0);
    CHECK(term.readAllStandardOutput().length() > 0);
  }
  SECTION("invalid pepp source") {
    QProcess term;
    term.start(path, {"get", "--ch", "00", "--fig", "x0", "--type", "pep"});
    wait_return(term, 1);
    auto err = term.readAllStandardError();
    err.replace("\r", "");
    CHECK(err == "Figure 00.x0 does not exist.\n");
  }
  SECTION("valid pepp source, invalid variant") {
    QProcess term;
    term.start(path, {"get", "--ch", "05", "--fig", "27", "--type", "not"});
    wait_return(term, 2);
    auto err = term.readAllStandardError();
    err.replace("\r", "");
    CHECK(err == "Figure 05.27 does not contain a \"not\" variant.\n");
  }
}
