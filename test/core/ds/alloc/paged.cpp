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

TEST_CASE("Slab variadic append is all-or-nothing", "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  // append(align, pad, fill, spans...) atomically append all spans at the desired alignment or throws.
  SECTION("A run that does not fit throws and writes nothing") {
    Page page(Page::MIN_PAGE_SIZE);
    const std::string filler(Page::MIN_PAGE_SIZE - 8, 'a');
    page.append_packed(span_of(filler));
    const auto before = page.used_capacity();
    REQUIRE(before == Page::MIN_PAGE_SIZE - 8);

    // First span fits the 8 remaining elements, the pair does not.
    const std::string head(6, 'b'), tail(6, 'c');
    REQUIRE(page.can_fit(span_of(head)));
    CHECK_FALSE(page.can_fit_all(0, 0, span_of(head), span_of(tail)));
    CHECK_THROWS_AS(page.append_packed(span_of(head), span_of(tail)), std::runtime_error);
    // Nothing was written, so the space is still there for a run that does fit.
    CHECK(page.used_capacity() == before);
  }
  SECTION("A run that fits is written back to back") {
    Page page(Page::MIN_PAGE_SIZE);
    const std::string head(6, 'b'), tail(2, 'c');
    auto at = page.append_packed(span_of(head), span_of(tail));
    CHECK(at == 0);
    CHECK(page.used_capacity() == head.size() + tail.size());
    CHECK(std::string(page.data(), page.used_capacity()) == head + tail);
  }
  SECTION("Per-span padding counts toward total size") {
    // With pad, each span costs more than its size, so a group of spans that seem to fit by summing their sizes will
    // not actually fit.
    Page page(Page::MIN_PAGE_SIZE);
    const std::string filler(Page::MIN_PAGE_SIZE - 4, 'a');
    page.append_packed(span_of(filler));
    const std::string a(2, 'b'), b(2, 'c');
    // 4 elements of data would fit exactly; 4 + one pad byte each does not.
    CHECK_FALSE(page.can_fit_all(0, 1, span_of(a), span_of(b)));
    CHECK_THROWS_AS(page.append(size_t{0}, size_t{1}, char{0}, span_of(a), span_of(b)), std::runtime_error);
    CHECK(page.used_capacity() == Page::MIN_PAGE_SIZE - 4);
  }
}

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
