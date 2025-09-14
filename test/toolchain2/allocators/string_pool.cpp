/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "toolchain2/allocators/string_pool.hpp"
#include <catch.hpp>

using Pool = pepp::tc::alloc::StringPool;
using String = pepp::tc::alloc::PooledString;

TEST_CASE("Allocator String Pooling", "[scope:asm][kind:unit][arch:pep10][!throws]") {
  static const std::string hi = "hi", world = "world";
  SECTION("Sequential insert/finds without pooling") {
    Pool p;
    auto handle_hi = p.insert(hi);
    CHECK(p.pooled_byte_size() == 2);
    CHECK(p.unpooled_byte_size() == 2);
    CHECK(p.count() == 1);
    CHECK(p.contains(hi));
    //  Adding \0 makes it a different string
    const char *hi_null = "hi\0";
    std::string_view hi_null_view(hi_null, 3);
    CHECK(!p.contains(hi_null_view));

    auto handle_world = p.insert(world);
    CHECK(p.unpooled_byte_size() == 7);
    CHECK(p.unpooled_byte_size() == 7);
    CHECK(p.count() == 2);
    CHECK(p.contains(world));
    // No gap between hi and world in page.
    CHECK(handle_hi.length() == handle_world.offset());

    CHECK(handle_world > handle_hi);

    // When combined with the next test case... it shows that order of insertion matters!
    auto handle_hiworld = p.insert(hi + world);
    // Inserted in wrong order, so no pooling possible without blindly probing memory.
    CHECK(p.pooled_byte_size() == 14);
    CHECK(p.unpooled_byte_size() == 14);
    CHECK(p.count() == 3);
    CHECK(handle_hiworld > handle_hi);
  }
  SECTION("Sequential insert/finds with pooling") {
    Pool p;
    // When combined with the next test case... it shows that order of insertion matters!
    auto handle_hiworld = p.insert(hi + world);
    CHECK(p.unpooled_byte_size() == 7);
    CHECK(p.unpooled_byte_size() == 7);
    CHECK(p.count() == 1);

    auto handle_hi = p.insert(hi);
    CHECK(p.pooled_byte_size() == 7);
    CHECK(p.unpooled_byte_size() == 9);
    CHECK(p.count() == 2);
    CHECK(p.contains(hi));

    auto handle_world = p.insert(world);
    CHECK(p.pooled_byte_size() == 7);
    CHECK(p.unpooled_byte_size() == 14);
    CHECK(p.count() == 3);

    // Null terminator still defeats pooling
    const char *world_null = "world\0";
    auto handle_world_null = p.insert(std::string_view(world_null, 6), Pool::AddNullTerminator::IfNotPresent);
    CHECK(p.pooled_byte_size() == 13);
    CHECK(p.unpooled_byte_size() == 20);
    CHECK(p.count() == 4);

    CHECK(handle_hiworld < handle_world_null);
    CHECK(handle_world_null > handle_world);
    CHECK(handle_world > handle_hi);
  }
  SECTION("Fallback to insertion order-sorting for strings of same length") {
    Pool p;
    // 'H' is lexicographically before 'h', so normally "Hi" would sort before "hi".
    CHECK((p.insert("hi") < p.insert("Hi")));
  }
  SECTION("Page probing for gaps") {
    Pool p;
    // Should allocate a new page for this small string.
    auto handle_hi = p.insert(hi);
    // Leave a few bytes at the end of a page for us to do a later insert.
    auto handle_large = p.insert(std::string(Pool::MIN_PAGE_SIZE - handle_hi.length() - 3, 'a'));
    CHECK(handle_hi.page() == handle_large.page());
    // World should allocate into a new page.
    auto handle_world = p.insert(world);
    CHECK(handle_world.page() > handle_large.page());
    // Short insert should go into the gap at the end of the first page.
    auto handle_bye = p.insert("bye");
    CHECK(handle_bye.page() == handle_hi.page());
    CHECK(handle_bye > handle_large);
    CHECK(handle_bye < handle_world);
    // All subsequent inserts MUST spill to second page
    auto handle_test = p.insert("t");
    CHECK(handle_test.page() == handle_world.page());
    CHECK(handle_test > handle_world);
  }
  SECTION("Throw if Allocacation exceeds max page size") {
    Pool p;
    REQUIRE_THROWS_AS(p.insert(std::string_view(std::string(Pool::MAX_PAGE_SIZE + 1, 'b'))), std::invalid_argument);
  }
}
