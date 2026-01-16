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

#include <QDebug>
#include <QString>
#include <catch.hpp>
#include "bts/libs/string_pool.hpp"

using Pool = pepp::bts::StringPool;
using String = pepp::bts::PooledString;

TEST_CASE("Allocator String Pooling", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  static const QString hi = "hi", world = "world";
  SECTION("Sequential insert/finds without pooling") {
    Pool p;
    auto handle_hi = p.insert(hi);
    CHECK(p.pooled_byte_size() == 2);
    CHECK(p.unpooled_byte_size() == 2);
    CHECK(p.count() == 1);
    CHECK(p.contains(hi));
    //  Adding \0 makes it a different string
    const char16_t *hi_null = u"hi\0";
    QStringView hi_null_view(hi_null, 3);
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
    const char16_t *world_null = u"world\0";
    auto handle_world_null = p.insert(QStringView(world_null, 6), Pool::AddNullTerminator::IfNotPresent);
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
    auto first = p.insert(u"hi");
    auto second = p.insert(u"Hi");
    CHECK(first < second);
  }
  SECTION("Page probing for gaps") {
    Pool p;
    // Should allocate a new page for this small string.
    auto handle_hi = p.insert(hi);
    // Leave a few bytes at the end of a page for us to do a later insert.
    auto handle_large = p.insert(QString(Pool::DEFAULT_PAGE_SIZE - handle_hi.length() - 3, 'a'));
    CHECK(handle_hi.page() == handle_large.page());
    // World should allocate into a new page.
    auto handle_world = p.insert(world);
    CHECK(handle_world.page() > handle_large.page());
    // Short insert should go into the gap at the end of the first page.
    auto handle_bye = p.insert(u"bye");
    CHECK(handle_bye.page() == handle_hi.page());
    CHECK(handle_bye > handle_large);
    CHECK(handle_bye < handle_world);
    // All subsequent inserts MUST spill to second page
    auto handle_test = p.insert(u"t");
    CHECK(handle_test.page() == handle_world.page());
    CHECK(handle_test > handle_world);
  }
  SECTION("Throw if Allocacation exceeds max page size") {
    Pool p;
    auto s = QString(Pool::MAX_PAGE_SIZE + 1, 'a');
    REQUIRE_THROWS_AS(p.insert(s), std::invalid_argument);
  }
  SECTION("Printing allocated strings does not crash") {
    Pool p;
    p.insert(u"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et "
             u"dolore magna aliqua");
    p.insert(u"Lorem ipsum dolor sit amet, consectetur adipiscing elit,");
    p.insert(u"em ipsum dolor sit amet");
    p.insert(u"amet, consectetur adipiscing elit");
    p.insert(u"ed do eiusmod tempor incididunt ut labore et dolore magna aliqua");
    p.insert(u"adipiscing elit, sed do");
    /*auto pages = pepp::tc::support::annotated_pages(p);
    CHECK(pages.size() == 1);
    auto as_str = pages[0].to_string();
    CHECK(as_str.size() > 4);*/
    // qDebug().noquote().nospace() << as_str;
    // CHECK(0);
  }
}
