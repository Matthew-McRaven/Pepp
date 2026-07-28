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
}