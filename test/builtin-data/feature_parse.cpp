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
#include "core/architectures.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "help/builtins/changelogmodel.hpp"

TEST_CASE("Feature parser", "[scope:help.bi][kind:unit][arch:*]") {
  using namespace bits;
  auto model = new ChangelogModel();
  CHECK(model->rowCount({}) > 0);
  SECTION("Empty") {

    CHECK(pepp::parse_features("") == pepp::Features::None);
    CHECK(pepp::parse_features("\t") == pepp::Features::None);
    CHECK(pepp::parse_features("\n") == pepp::Features::None);
  }
  SECTION("Single, Valid Features") {
    CHECK(pepp::parse_features("OneByte") == pepp::Features::OneByte);
    CHECK(pepp::parse_features("TwoByte") == pepp::Features::TwoByte);
    CHECK(pepp::parse_features("noos") == pepp::Features::NoOS);
  }
  SECTION("Multiple, Valid Features") {
    CHECK(pepp::parse_features("OneByte,TwoByte") == (pepp::Features::OneByte | pepp::Features::TwoByte));
    CHECK(pepp::parse_features("NoOS,\t\tOneByte") == (pepp::Features::OneByte | pepp::Features::NoOS));
    CHECK(pepp::parse_features("TwoByte,\tNoOS") == (pepp::Features::TwoByte | pepp::Features::NoOS));
    CHECK(pepp::parse_features("OneByte  ,  TwoByte,   NoOS") ==
          (pepp::Features::OneByte | pepp::Features::TwoByte | pepp::Features::NoOS));
  }
  SECTION("Invalid Features") {
    CHECK(pepp::parse_features("Invalid") == pepp::Features::None);
    CHECK(pepp::parse_features("OneByte,  Invalid") == pepp::Features::OneByte);
    CHECK(pepp::parse_features("Invalid,  TwoByte") == pepp::Features::TwoByte);
    CHECK(pepp::parse_features("OneByte,  Invalid,NoOS") == (pepp::Features::OneByte | pepp::Features::NoOS));
  }
}
