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

#include "core/ds/string_pool.hpp"
#include <catch/catch.hpp>

using Pool = pepp::bts::StringPool;
using String = pepp::bts::PooledString;
using Terminated = pepp::bts::NullTerminated;

namespace {
// A string_view that includes its own terminator.
std::string_view with_nul(const std::string &str) { return std::string_view(str.data(), str.size() + 1); }
} // namespace

TEST_CASE("Allocator String Pooling", "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
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
    auto handle_world_null = p.insert(std::string_view(world_null, 6));
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
    auto first = p.insert("hi");
    auto second = p.insert("Hi");
    CHECK(first < second);
  }
  SECTION("Page probing for gaps") {
    Pool p;
    // Should allocate a new page for this small string.
    auto handle_hi = p.insert(hi);
    // Leave a few bytes at the end of a page for us to do a later insert.
    auto handle_large = p.insert(std::string(Pool::DEFAULT_PAGE_SIZE - handle_hi.length() - 3, 'a'));
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
    auto s = std::string(Pool::MAX_PAGE_SIZE + 1, 'a');
    REQUIRE_THROWS_AS(p.insert(s), std::invalid_argument);
  }
  SECTION("Printing allocated strings does not crash") {
    Pool p;
    p.insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et "
             "dolore magna aliqua");
    p.insert("Lorem ipsum dolor sit amet, consectetur adipiscing elit,");
    p.insert("em ipsum dolor sit amet");
    p.insert("amet, consectetur adipiscing elit");
    p.insert("ed do eiusmod tempor incididunt ut labore et dolore magna aliqua");
    p.insert("adipiscing elit, sed do");
    /*auto pages = pepp::tc::support::annotated_pages(p);
    CHECK(pages.size() == 1);
    auto as_str = pages[0].to_string();
    CHECK(as_str.size() > 4);*/
    // qDebug().noquote().nospace() << as_str;
    // CHECK(0);
  }
  SECTION("Strings remain findable past the first page") {
    Pool p;
    std::vector<std::string> names;
    std::vector<String> handles;
    for (int it = 0; it < 4000; it++) names.push_back("symbol_number_" + std::to_string(it));
    for (const auto &name : names) handles.push_back(p.insert_null_terminated(name));
    REQUIRE(p.pooled_byte_size() > Pool::DEFAULT_PAGE_SIZE);
    for (size_t it = 0; it < names.size(); it++) {
      INFO("index: " << it);
      auto found = p.find(Terminated{names[it]});
      REQUIRE(found.has_value());
      // A handle stays valid, and keeps its offset, across every later insertion.
      CHECK(*found == handles[it]);
      CHECK(p.byte_offset(*found) == p.byte_offset(handles[it]));
      auto str = p.find(handles[it]);
      REQUIRE(str.has_value());
      CHECK(str->substr(0, str->size() - 1) == names[it]);
    }
  }
}

TEST_CASE("String pool with null-termination", "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  SECTION("Null terminator included in length") {
    Pool p;
    auto handle = p.insert_null_terminated("main");
    CHECK(handle.length() == 5);
    CHECK(p.pooled_byte_size() == 5);
    auto str = p.find(handle);
    REQUIRE(str.has_value());
    CHECK(str->size() == 5);
    CHECK(str->back() == '\0');
    CHECK(str->substr(0, 4) == "main");
    CHECK(p.byte_offset(handle) == 0);
  }
  SECTION("Inserting the same string twice yields the same handle") {
    // A previous bug in allocated() would cause all null-terminated inserts to re-allocate on future inserts.
    Pool p;
    auto first = p.insert_null_terminated("main");
    auto second = p.insert_null_terminated("main");
    CHECK(first == second);
    CHECK(p.count() == 1);
    CHECK(p.pooled_byte_size() == 5);
  }
  SECTION("Already-included null terminator") {
    Pool p;
    static const std::string main = "main";
    auto bare = p.insert_null_terminated(main);
    auto carried = p.insert_null_terminated(with_nul(main));
    CHECK(bare == carried);
    CHECK(p.count() == 1);
  }
  SECTION("Empty string is a single null") {
    Pool p;
    auto handle = p.insert_null_terminated("");
    CHECK(handle.length() == 1);
    CHECK(p.byte_offset(handle) == 0);
    auto str = p.find(handle);
    REQUIRE(str.has_value());
    CHECK(str->size() == 1);
    CHECK(str->front() == '\0');
    // Re-inserting finds it rather than allocating a second terminator.
    CHECK(p.insert_null_terminated("") == handle);
    CHECK(p.pooled_byte_size() == 1);
  }
}

TEST_CASE("String pool null-terminated sharing", "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  SECTION("A tail shares the container's terminator") {
    Pool p;
    auto container = p.insert_null_terminated("mainCln"); // "mainCln\0"
    CHECK(p.pooled_byte_size() == 8);
    auto tail = p.insert_null_terminated("Cln");
    // Shared: no new bytes, and it points four bytes into the container.
    CHECK(p.pooled_byte_size() == 8);
    CHECK(p.byte_offset(tail) == p.byte_offset(container) + 4);
    CHECK(tail.length() == 4);
    auto str = p.find(tail);
    REQUIRE(str.has_value());
    CHECK(str->size() == 4);
    CHECK(str->back() == '\0');
    CHECK(str->substr(0, 3) == "Cln");
  }
  SECTION("Avoid sharing in the middle of another container") {
    // "main" is a prefix of "mainCln", but cannot pool because of the missing terminator.
    Pool p;
    auto container = p.insert_null_terminated("mainCln");
    auto prefix = p.insert_null_terminated("main");
    CHECK(p.byte_offset(prefix) != p.byte_offset(container));
    CHECK(p.pooled_byte_size() == 8 + 5);
    CHECK(p.count() == 2);
    // Read back the same way C would, which is up until first null terminator.
    auto str = p.find(prefix);
    REQUIRE(str.has_value());
    CHECK(str->substr(0, str->find('\0')) == "main");
  }
  SECTION("Later writes do not clobber earlier ones") {
    Pool p;
    auto tail = p.insert_null_terminated("Cln");
    auto container = p.insert_null_terminated("mainCln");
    // "Cln" was allocated first, so the container may not combine the two.
    CHECK(p.find(tail)->substr(0, 3) == "Cln");
    CHECK(p.find(container)->substr(0, 7) == "mainCln");
  }
  SECTION("Terminated and un-terminated paths do not collide") {
    // Lengths differ (4 vs 5), so these are distinct keys and neither displaces the other.
    Pool p;
    auto bare = p.insert("main");
    auto terminated = p.insert_null_terminated("main");
    CHECK(bare != terminated);
    CHECK(bare.length() == 4);
    CHECK(terminated.length() == 5);
    CHECK(p.count() == 2);
    // Each is reachable by the key that matches it, and only by that key.
    CHECK(p.find(std::string_view("main")).has_value());
    CHECK(*p.find(std::string_view("main")) == bare);
    CHECK(p.find(Terminated{"main"}).has_value());
    CHECK(*p.find(Terminated{"main"}) == terminated);
  }
}

TEST_CASE("String pool NullTerminated ordering", "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  // The comparator gained overloads that treat a trailing '\0' as present without it being stored.
  // They must order identically to the previous forms.
  SECTION("A NullTerminated key finds what the materialized form finds") {
    Pool p;
    static const std::vector<std::string> names = {"a", "main", "mainCln", "", "zzz", "a_longer_symbol_name"};
    for (const auto &name : names) p.insert_null_terminated(name);
    for (const auto &name : names) {
      INFO("name: " << name);
      auto by_key = p.find(Terminated{name});
      auto by_materialized = p.find(with_nul(name));
      REQUIRE(by_key.has_value());
      REQUIRE(by_materialized.has_value());
      CHECK(*by_key == *by_materialized);
    }
  }
  SECTION("Absent strings") {
    Pool p;
    p.insert_null_terminated("main");
    static const std::string absent = "absent";
    CHECK(!p.find(Terminated{absent}).has_value());
    CHECK(!p.find(with_nul(absent)).has_value());
    // A bare key must not match the terminated entry.
    CHECK(!p.find(std::string_view("main")).has_value());
  }
}

TEST_CASE("String pool comparator context", "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  // Less holds a raw pointer to the pool it resolves handles against.
  SECTION("A default-constructed pool compares against itself") {
    Pool p;
    CHECK(p.comparator_context() == &p);
  }
  SECTION("with_null_entry() returns a pool that compares against itself") {
    // If copy elision fails, this would be a dangling pointer.
    auto p = Pool::with_null_entry();
    CHECK(p.comparator_context() == &p);
    // And the seeded entry is reachable, which it would not be if the comparator were wrong.
    REQUIRE(p.find(Terminated{""}).has_value());
    CHECK(p.byte_offset(*p.find(Terminated{""})) == 0);
  }
}

