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

#include <QTemporaryDir>
#include <catch.hpp>
#include "help/builtins/registry.hpp"

QString toc = "{"
              "\"bookName\" : \"Test\","
              "\"dbVersion\" : \"2\""
              "}";

TEST_CASE("Registry using external data", "[scope:help.bi][kind:unit][arch:*]") {
  SECTION("Loads manifest from a directory") {
    QTemporaryDir dir;
    REQUIRE(QDir(dir.path()).mkdir("csde"));
    QFile toc_file(dir.filePath("csde/toc.json"));
    REQUIRE(toc_file.open(QIODevice::WriteOnly));
    toc_file.write(toc.toUtf8());
    toc_file.close();
    auto reg = builtins::Registry(nullptr, dir.path());
    REQUIRE(reg.books().size() == 1);
  }
  SECTION("Can load default books") {
    auto reg = builtins::Registry(nullptr, builtins::default_book_path);
    REQUIRE(reg.books().size() == 3);
    // TODO: CS4E still has no figures and would fail the next line.
    // for (const auto &book : reg.books()) CHECK(!book->figures().empty());
  }
  SECTION("Does not crash on malformed TOC") {
    QTemporaryDir dir;
    REQUIRE(QDir(dir.path()).mkdir("csde"));
    QFile toc_file(dir.filePath("csde/toc.json"));
    REQUIRE(toc_file.open(QIODevice::WriteOnly));
    toc_file.write("{");
    toc_file.close();
    auto reg = builtins::Registry(nullptr, dir.path());
    REQUIRE(reg.books().size() == 0);
  }
  SECTION("Does not crash on malformed figure") {
    QTemporaryDir dir;
    REQUIRE(QDir(dir.path()).mkdir("csde"));
    QFile toc_file(dir.filePath("csde/toc.json"));
    REQUIRE(toc_file.open(QIODevice::WriteOnly));
    toc_file.write(toc.toUtf8());
    toc_file.close();
    REQUIRE(QDir(dir.path()).mkdir("csde/ch01"));
    QFile figure_file(dir.filePath("csde/ch01/figure.json"));
    REQUIRE(figure_file.open(QIODevice::WriteOnly));
    figure_file.write("{");
    figure_file.close();
    auto reg = builtins::Registry(nullptr, dir.path());
    REQUIRE(reg.books().size() == 1);
    auto book = reg.findBook("Test");
    REQUIRE(book != nullptr);
    REQUIRE(book->figures().empty());
  }
}
