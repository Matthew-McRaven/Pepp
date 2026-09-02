/*
 * Copyright (c) 2025 J. Stanley Warford, Matthew McRaven
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

#include "sim/debug/expr_eval.hpp"

TEST_CASE("Integer promotion rules", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug::types;
  using T = Primitives;
  CHECK(common_type(T::i8, T::i8) == T::i8);
  CHECK(common_type(T::i8, T::u8) == T::u8);
  CHECK(common_type(T::u8, T::i8) == T::u8);
  CHECK(common_type(T::u8, T::u8) == T::u8);

  CHECK(common_type(T::i8, T::i16) == T::i16);
  CHECK(common_type(T::i8, T::u16) == T::u16);
  CHECK(common_type(T::u8, T::i16) == T::i16);
  CHECK(common_type(T::u8, T::u16) == T::u16);

  CHECK(common_type(T::i8, T::i32) == T::i32);
  CHECK(common_type(T::i8, T::u32) == T::u32);
  CHECK(common_type(T::u8, T::i32) == T::i32);
  CHECK(common_type(T::u8, T::u32) == T::u32);

  CHECK(common_type(T::i16, T::i8) == T::i16);
  CHECK(common_type(T::i16, T::u8) == T::i16);
  CHECK(common_type(T::u16, T::i8) == T::u16);
  CHECK(common_type(T::u16, T::u8) == T::u16);

  CHECK(common_type(T::i16, T::i16) == T::i16);
  CHECK(common_type(T::i16, T::u16) == T::u16);
  CHECK(common_type(T::u16, T::i16) == T::u16);
  CHECK(common_type(T::u16, T::u16) == T::u16);

  CHECK(common_type(T::i16, T::i32) == T::i32);
  CHECK(common_type(T::i16, T::u32) == T::u32);
  CHECK(common_type(T::u16, T::i32) == T::i32);
  CHECK(common_type(T::u16, T::u32) == T::u32);

  CHECK(common_type(T::i32, T::i8) == T::i32);
  CHECK(common_type(T::i32, T::u8) == T::i32);
  CHECK(common_type(T::u32, T::i8) == T::u32);
  CHECK(common_type(T::u32, T::u8) == T::u32);

  CHECK(common_type(T::i32, T::i16) == T::i32);
  CHECK(common_type(T::i32, T::u16) == T::i32);
  CHECK(common_type(T::u32, T::i16) == T::u32);
  CHECK(common_type(T::u32, T::u16) == T::u32);

  CHECK(common_type(T::i32, T::i32) == T::i32);
  CHECK(common_type(T::i32, T::u32) == T::u32);
  CHECK(common_type(T::u32, T::i32) == T::u32);
  CHECK(common_type(T::u32, T::u32) == T::u32);
}
