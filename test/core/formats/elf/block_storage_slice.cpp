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
#include <algorithm>
#include <array>
#include <catch.hpp>
#include <memory>
#include "core/formats/elf/packed_ops.hpp"
#include "core/formats/elf/packed_storage.hpp"

TEST_CASE("BlockStorage can be sliced", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using Slice = BlockStorage::BlockStorageSlice;

  SECTION("Two slices sharing a BlockStorage") {
    auto block = std::make_shared<BlockStorage>();
    block->allocate(6);
    Slice text(block, 0, 4), data(block, 4, 2);
    const std::array<u8, 4> text_bytes{1, 2, 3, 4};
    const std::array<u8, 2> data_bytes{5, 6};
    text.set(0, text_bytes);
    data.set(0, data_bytes);

    // Each slice sees its own bytes.
    CHECK(text.size() == 4);
    CHECK(data.size() == 2);
    CHECK(std::ranges::equal(text.get(0, 4), text_bytes));
    CHECK(std::ranges::equal(data.get(0, 2), data_bytes));
    CHECK(std::ranges::equal(block->get(0, 6), std::array<u8, 6>{1, 2, 3, 4, 5, 6}));

    // Slices shares memory with parent rather than copying it.
    CHECK(data.get(0, 1).data() == block->get(4, 1).data());
    block->set(5, std::array<u8, 1>{9});
    CHECK(data.get(1, 1)[0] == 9);

    // A slice cannot allow out-of-bounds access.
    CHECK_THROWS_AS(text.set(3, data_bytes), std::out_of_range);
    CHECK(text.get(3, 2).empty());

    // Laid out as separate sections, both point into the one buffer.
    std::vector<LayoutItem> layout;
    const auto end = data.calculate_layout(layout, text.calculate_layout(layout, 0x40));
    REQUIRE(layout.size() == 2);
    CHECK(layout[0].offset == 0x40);
    CHECK(layout[0].data.data() == block->get(0, 4).data());
    CHECK(layout[1].offset == 0x44);
    CHECK(layout[1].data.data() == block->get(4, 2).data());
    CHECK(end == 0x46);
  }
  SECTION("Clearing a slice zeroes only its bytes") {
    auto block = std::make_shared<BlockStorage>();
    block->append(std::array<u8, 4>{1, 2, 3, 4});
    Slice slice(block, 1, 2);
    slice.clear();
    CHECK(slice.size() == 2);
    CHECK(std::ranges::equal(block->get(0, 4), std::array<u8, 4>{1, 0, 0, 4}));
  }
  SECTION("A slice must fit within its parent") {
    auto block = std::make_shared<BlockStorage>();
    block->allocate(4);
    CHECK_NOTHROW(Slice(block, 4, 0));
    CHECK_THROWS_AS(Slice(block, 3, 2), std::invalid_argument);
    CHECK_THROWS_AS(Slice(block, 5, 0), std::invalid_argument);
    CHECK_THROWS_AS(Slice(nullptr, 0, 0), std::invalid_argument);
  }
  SECTION("strlen past the end of a slice is 0") {
    auto block = std::make_shared<BlockStorage>();
    block->append(std::array<u8, 4>{'a', 'b', 0, 'c'});
    Slice slice(block, 0, 3);
    CHECK(slice.strlen(0) == 2);
    CHECK(slice.strlen(3) == 0);
    CHECK(slice.strlen(100) == 0);
  }
}
