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
#include "core/ds/alloc/paged.hpp"
#include <catch.hpp>
#include <string>

namespace {
using Alloc = pepp::bts::PagedAllocator<char>;
using Page = pepp::bts::Slab<char>;

bits::span<const char> span_of(const std::string &str) { return bits::span<const char>{str.data(), str.size()}; }

// Every page's base is the total used capacity of the pages before it. offset_for_indices, and so
// StringPool::byte_offset above it, are only meaningful while that holds.
void check_page_bases(const Alloc &alloc) {
  size_t running = 0;
  for (size_t page = 0; page < alloc.page_count(); page++) {
    INFO("page " << page);
    CHECK(alloc.offset_for_indices({.index = page, .offset = 0}) == running);
    running += alloc.page(page).used_capacity();
  }
  CHECK(alloc.size() == running);
}
} // namespace

TEST_CASE("PagedAllocator page bases track padded allocations",
          "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  SECTION("A padded insert into an earlier page moves later pages by the padded size") {
    // insert() is first-fit, so once a second page exists an allocation can still land in a gap left
    // at the end of an earlier one.
    Alloc alloc;
    constexpr size_t gap = 8;

    const std::string filler(Page::DEFAULT_PAGE_SIZE - gap, 'a');
    alloc.insert(span_of(filler));
    REQUIRE(alloc.page_count() == 1);
    REQUIRE(alloc.page(0).used_capacity() == Page::DEFAULT_PAGE_SIZE - gap);

    // Too large for the gap, so this opens a second page.
    const std::string spill(gap * 4, 'b');
    auto spilled = alloc.insert(span_of(spill));
    REQUIRE(alloc.page_count() == 2);
    REQUIRE(spilled.indices.index == 1);
    check_page_bases(alloc);

    // Small enough for the gap even once padded, so it goes back into page 0.
    const std::string tiny(3, 'c');
    auto padded = alloc.insert(span_of(tiny), 0, 1, 0);
    REQUIRE(padded.indices.index == 0);
    // Page 0 grew by the string *and* its padding.
    CHECK(alloc.page(0).used_capacity() == Page::DEFAULT_PAGE_SIZE - gap + tiny.size() + 1);
    check_page_bases(alloc);

    // The allocation living on page 1 must still resolve to its own first byte.
    CHECK(alloc.offset_for_indices(spilled.indices) == alloc.page(0).used_capacity() + spilled.indices.offset);
    auto round_trip = alloc.indices_for_offset(alloc.offset_for_indices(spilled.indices));
    CHECK(round_trip.index == spilled.indices.index);
    CHECK(round_trip.offset == spilled.indices.offset);
  }
  SECTION("Bases stay consistent across repeated padded inserts into a gap") {
    Alloc alloc;
    constexpr size_t gap = 64;
    const std::string filler(Page::DEFAULT_PAGE_SIZE - gap, 'a');
    alloc.insert(span_of(filler));
    const std::string spill(gap * 4, 'b');
    auto spilled = alloc.insert(span_of(spill));
    REQUIRE(alloc.page_count() == 2);

    // Each of these drifts the later base by one more byte if padding is not accounted for.
    const std::string tiny(3, 'c');
    for (int it = 0; it < 8; it++) {
      auto placed = alloc.insert(span_of(tiny), 0, 1, 0);
      REQUIRE(placed.indices.index == 0);
    }
    check_page_bases(alloc);
    CHECK(alloc.offset_for_indices(spilled.indices) == alloc.page(0).used_capacity());
  }
}
