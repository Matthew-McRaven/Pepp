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
TEST_CASE("Buffer ids do not alias across reuse",
          "[kind:unit][arch:*][!throws][tc2][scope:core][scope:core.ds]") {
  using namespace pepp::bts;
  // Indices have to be reusable -- ids are 16 bits and a long session churns through far more than 65536 buffers --
  // but an id that outlives the buffer it named must not resolve to whoever occupies that slot afterwards. That
  // failure mode is silent: the stale reference reads someone else's bytes and nothing reports it.
  const auto mgr = std::make_shared<BufferManager>();

  SECTION("A freed id stops resolving, and does not name its successor") {
    auto *first = mgr->alloc_buffer();
    const auto stale = first->id();
    REQUIRE(mgr->find(stale) == first);

    mgr->free_buffer(stale);
    CHECK(mgr->find(stale) == nullptr);

    // The storage comes back -- that is the point of the free list -- but under a different id.
    auto *second = mgr->alloc_buffer();
    CHECK(second == first);
    CHECK(second->id() != stale);
    CHECK(mgr->find(second->id()) == second);
    // The whole property in one line: the old id does not name the new occupant.
    CHECK(mgr->find(stale) == nullptr);
  }

  SECTION("Index and generation are separable") {
    auto *buf = mgr->alloc_buffer();
    const auto first = buf->id();
    mgr->free_buffer(first);
    auto *again = mgr->alloc_buffer();
    // Same slot, so the same index; a new occupant, so a new generation.
    CHECK(Buffer::index_of(again->id()) == Buffer::index_of(first));
    CHECK(Buffer::generation_of(again->id()) != Buffer::generation_of(first));
  }

  SECTION("Index 0 is never handed out, so ID{0} always means no buffer") {
    CHECK(mgr->find(Buffer::ID{0}) == nullptr);
    for (int i = 0; i < 8; ++i) CHECK(Buffer::index_of(mgr->alloc_buffer()->id()) != 0);
    CHECK(mgr->find(Buffer::ID{0}) == nullptr);
  }

  SECTION("Freeing through a stale id does not evict the current occupant") {
    auto *first = mgr->alloc_buffer();
    const auto stale = first->id();
    mgr->free_buffer(stale);
    auto *second = mgr->alloc_buffer();
    REQUIRE(second->id() != stale);

    mgr->free_buffer(stale); // second free of an id nobody holds any more
    CHECK(mgr->find(second->id()) == second);
    CHECK(mgr->allocated_buffers() == 1);
  }

  SECTION("Free indices are reused round-robin, not last-in-first-out") {
    // LIFO would cycle one slot's generation on every free/alloc pair and burn through the generation space fast.
    // Round-robin spreads reuse across every free index first, which is what makes a short generation field enough.
    auto *a = mgr->alloc_buffer();
    auto *b = mgr->alloc_buffer();
    auto *c = mgr->alloc_buffer();
    const auto ia = Buffer::index_of(a->id()), ib = Buffer::index_of(b->id()), ic = Buffer::index_of(c->id());
    mgr->free_buffer(a->id());
    mgr->free_buffer(b->id());
    mgr->free_buffer(c->id());
    CHECK(mgr->free_buffers() == 3);

    CHECK(Buffer::index_of(mgr->alloc_buffer()->id()) == ia);
    CHECK(Buffer::index_of(mgr->alloc_buffer()->id()) == ib);
    CHECK(Buffer::index_of(mgr->alloc_buffer()->id()) == ic);
    CHECK(mgr->free_buffers() == 0);
  }
}
