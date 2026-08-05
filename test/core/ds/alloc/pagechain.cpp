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
#include "core/ds/alloc/pagechain.hpp"
#include <catch.hpp>
TEST_CASE("Validations of Buffer chain classes", "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  using namespace pepp::bts;
  SECTION("Allocate a chain") {
    const auto mgr = std::make_shared<BufferManager>();
    CHECK(mgr->free_buffers() == 0);
    CHECK(mgr->allocated_buffers() == 0);
    {
      // Allocating chain does not pre-allocate buffer
      const auto chain = mgr->alloc_chain();
      CHECK(mgr->free_buffers() == 0);
      CHECK(mgr->allocated_buffers() == 0);
      // Allocated on first access
      chain->append(std::array<u8, 2>{0xfe, 0xed});
      CHECK(mgr->free_buffers() == 0);
      CHECK(mgr->allocated_buffers() == 1);
    }
    // And freed when chain falls out of scope.
    CHECK(mgr->free_buffers() == 1);
    CHECK(mgr->allocated_buffers() == 0);
  }
  SECTION("Allocate and free a buffer") {
    const auto mgr = std::make_shared<BufferManager>();
    auto buf = mgr->alloc_buffer();
    CHECK(buf != nullptr);
    CHECK(mgr->free_buffers() == 0);
    CHECK(mgr->allocated_buffers() == 1);
    auto find = mgr->find(buf->id());
    CHECK(find == buf);
    mgr->free_buffer(buf->id());
    CHECK(mgr->free_buffers() == 1);
    CHECK(mgr->allocated_buffers() == 0);
  }
  SECTION("BufferChain builds a chain") {
    const auto mgr = std::make_shared<BufferManager>();
    const auto chain = mgr->alloc_chain();
    CHECK(chain->buffer_count() == 0);
    CHECK(mgr->allocated_buffers() == 0);
    chain->allocate_uninitialized(Buffer::SIZE);
    CHECK(chain->buffer_count() == 1);
    CHECK(mgr->allocated_buffers() == 1);
    CHECK(chain->buffer(0)->id() != Buffer::ID{0});
    CHECK(chain->successor(chain->buffer(0)->id()) == Buffer::ID{0});
    chain->allocate_uninitialized(Buffer::SIZE);
    CHECK(mgr->allocated_buffers() == 2);
    CHECK(chain->buffer_count() == 2);
    CHECK(chain->buffer(1)->id() != Buffer::ID{0});
    CHECK(chain->successor(chain->buffer(0)->id()) == chain->buffer(1)->id());
    chain->clear();
    CHECK(mgr->allocated_buffers() == 0);
    CHECK(mgr->free_buffers() == 2);
  }
  SECTION("A recycled buffer comes back empty") {
    const auto mgr = std::make_shared<BufferManager>();
    {
      const auto chain = mgr->alloc_chain();
      // Use every byte, so a buffer that kept its allocation cursor would report itself full on the way back out.
      chain->allocate_uninitialized(Buffer::SIZE);
      CHECK(mgr->allocated_buffers() == 1);
    }
    REQUIRE(mgr->free_buffers() == 1);

    // Reuse must start from zero. If the cursor came back with the buffer, can_fit() fails, the chain rolls onto a
    // second buffer, and buffer_count() gives it away -- which is what used to happen, all the way up to throwing
    // "Page overflow" once the pool had no clean buffers left.
    const auto chain = mgr->alloc_chain();
    auto loc = chain->append(std::array<u8, 4>{1, 2, 3, 4});
    CHECK(loc.offset == 0);
    CHECK(chain->buffer_count() == 1);
    CHECK(mgr->free_buffers() == 0);
  }
  SECTION("reserve hands back a writable view of the bytes it allocated") {
    const auto mgr = std::make_shared<BufferManager>();
    const auto chain = mgr->alloc_chain();
    auto first = chain->reserve(4);
    REQUIRE(first.bytes.size() == 4);
    first.bytes[0] = 0xAA, first.bytes[3] = 0xDD;

    // The span has to alias the chain's own storage. A copy would compile and silently record nothing.
    auto *buf = chain->buffer(first.loc.id);
    REQUIRE(buf != nullptr);
    CHECK(buf->span()[first.loc.offset + 0] == 0xAA);
    CHECK(buf->span()[first.loc.offset + 3] == 0xDD);

    // And a second reservation must not overlap the first.
    auto second = chain->reserve(4);
    CHECK(second.loc.offset == first.loc.offset + 4);
  }
}