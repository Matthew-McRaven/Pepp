/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "sim3/subsystems/ram/broadcast/pubsub.hpp"
#include <catch.hpp>

TEST_CASE("Publish-subscribe", "[scope:sim][kind:unit][arch:*]") {
  SECTION("1 producer, 0 consumers. publish") {
    auto channel = QSharedPointer<sim::memory::detail::Channel<quint8, quint8>>::create(0);
    auto endpoint = channel->new_endpoint();
    REQUIRE_NOTHROW(endpoint->append_value(0x25));
  }
  SECTION("1 producer, 1 consumer. publish; read") {
    auto channel = QSharedPointer<sim::memory::detail::Channel<quint8, quint8>>::create(0);
    auto publish = channel->new_endpoint();
    auto subscribe = channel->new_endpoint();

    // Check that we can write/read a value.
    publish->append_value(0x25);
    auto value = subscribe->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0x25);
  }
  SECTION("1 producer, 1 consumer. publish; read; revert") {
    auto channel = QSharedPointer<sim::memory::detail::Channel<quint8, quint8>>::create(0);
    auto publish = channel->new_endpoint();
    auto subscribe = channel->new_endpoint();

    // Check that we can read a value from a singl producer and we can revert
    // it.
    publish->append_value(0x25);
    auto value = subscribe->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0x25);
    publish->unwrite();
    value = subscribe->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0);
  }
  SECTION("2 producers, 1 consumer. publish; revert") {
    auto channel = QSharedPointer<sim::memory::detail::Channel<quint8, quint8>>::create(0);
    auto publish0 = channel->new_endpoint();
    auto publish1 = channel->new_endpoint();
    auto subscribe = channel->new_endpoint();
    publish0->append_value(0x25);
    publish1->append_value(0x10);

    // Read and check both values.
    auto value = subscribe->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0x25);
    value = subscribe->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0x10);

    // Check that we are reset to the root upon undoing publish0's write.
    publish0->unwrite();
    value = subscribe->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0);
  }
  SECTION("2 producers, 1 consumer. publish; unread") {
    auto channel = QSharedPointer<sim::memory::detail::Channel<quint8, quint8>>::create(0);
    auto publish0 = channel->new_endpoint();
    auto publish1 = channel->new_endpoint();
    auto subscribe0 = channel->new_endpoint();
    publish0->append_value(0x25);
    publish1->append_value(0x10);

    // Read and check both values.
    auto value = subscribe0->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0x25);
    value = subscribe0->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0x10);

    // Check that we can unread a value.
    subscribe0->unread();
    value = subscribe0->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0x10);

    // Check that unwrite works after unread.
    publish0->unwrite();
    value = subscribe0->next_value();
    REQUIRE(value.has_value());
    CHECK(*value == 0);
  }
}
