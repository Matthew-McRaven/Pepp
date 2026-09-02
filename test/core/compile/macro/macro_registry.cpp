/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/compile/macro/macro_registry.hpp"
#include <catch/catch.hpp>

TEST_CASE("Macro Registry v2", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  using MR = pepp::tc::MacroRegistry;
  using MD = pepp::tc::MacroDefinition;
  using SM = MR::SearchMode;
  SECTION("Contains (only in child)") {
    auto parent = std::make_shared<MR>();
    auto child = std::make_shared<MR>(parent);
    auto md = std::make_shared<MD>();
    md->name = "feed";
    md->body = "beef";
    CHECK(child->insert(md));
    CHECK(child->contains("feed", SM::Local));
    CHECK(!child->contains("feed", SM::Parent));
    CHECK(child->contains("feed", SM::LocalThenParent));
    CHECK(!parent->contains("feed", SM::Local));
  }
  SECTION("Contains (both child && parent)") {
    auto parent = std::make_shared<MR>();
    auto child = std::make_shared<MR>(parent);
    auto md_c = std::make_shared<MD>();
    md_c->name = "feed";
    md_c->body = "beef";
    auto md_p = std::make_shared<MD>();
    md_p->name = "feed";
    md_p->body = "beef";
    CHECK(parent->insert(md_p));
    CHECK(!child->insert(md_c, SM::LocalThenParent));
    CHECK(child->insert(md_c, SM::Local));

    CHECK(child->find("feed", SM::Local) == md_c);
    CHECK(child->find("feed", SM::Parent) == md_p);
    CHECK(child->find("feed", SM::LocalThenParent) == md_c);
    CHECK(parent->find("feed", SM::Local) == md_p);
  }

  SECTION("Contains (both child && parent)") {
    auto parent = std::make_shared<MR>();
    auto child = std::make_shared<MR>(parent);
    auto md_c = std::make_shared<MD>();
    md_c->name = "feed";
    md_c->body = "beef";
    auto md_p = std::make_shared<MD>();
    md_p->name = "feed";
    md_p->body = "beef";
    CHECK(parent->insert(md_p));
    CHECK(!child->insert(md_c, SM::LocalThenParent));
    CHECK(child->insert(md_c, SM::Local));

    CHECK(child->find("feed", SM::Local) == md_c);
    CHECK(child->find("feed", SM::Parent) == md_p);
    CHECK(child->find("feed", SM::LocalThenParent) == md_c);
    CHECK(parent->find("feed", SM::Local) == md_p);
  }

  SECTION("Purge in child ") {
    auto parent = std::make_shared<MR>();
    auto child = std::make_shared<MR>(parent);
    auto md_c = std::make_shared<MD>();
    md_c->name = "feed";
    md_c->body = "beef";
    auto md_p = std::make_shared<MD>();
    md_p->name = "feed";
    md_p->body = "beef";
    CHECK(parent->insert(md_p));
    CHECK(!child->insert(md_c, SM::LocalThenParent));
    CHECK(child->contains("feed"));
    child->purge("feed");
    CHECK(!child->contains("feed"));
    CHECK(child->find("feed", SM::Local) == nullptr);
    CHECK(child->insert(md_c, SM::LocalThenParent));

    CHECK(child->find("feed", SM::Local) == md_c);
    CHECK(child->find("feed", SM::Parent) == md_p);
    CHECK(child->find("feed", SM::LocalThenParent) == md_c);
    CHECK(parent->find("feed", SM::Local) == md_p);
  }
}
