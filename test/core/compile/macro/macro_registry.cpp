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

  SECTION("Definitions are found by argument count") {
    auto registry = std::make_shared<MR>();
    auto one = std::make_shared<MD>(MD{"call", {{.name = "sym"}}, "one"});
    auto two = std::make_shared<MD>(MD{"call", {{.name = "rd"}, {.name = "sym"}}, "two"});
    CHECK(registry->insert(one));
    CHECK(registry->insert(two));
    // Only a definition with the same number of arguments conflicts.
    CHECK(!registry->insert(std::make_shared<MD>(MD{"call", {{.name = "other"}}, "three"})));
    CHECK(registry->find("call", 1) == one);
    CHECK(registry->find("call", 2) == two);
    CHECK(registry->find("call", 0) == nullptr);
    CHECK(registry->find("call", 3) == nullptr);
    // Purging a name removes every arity.
    registry->purge("call");
    CHECK(!registry->contains("call"));
    CHECK(registry->find("call", 1) == nullptr);
  }
  SECTION("Trailing arguments with defaults may be omitted") {
    auto registry = std::make_shared<MR>();
    auto defaults = std::make_shared<MD>(MD{"m", {{.name = "a"}, {.name = "b", .default_value = "0"}}, ""});
    CHECK(registry->insert(defaults));
    CHECK(registry->find("m", 0) == nullptr);
    CHECK(registry->find("m", 1) == defaults);
    CHECK(registry->find("m", 2) == defaults);
    // An exact number of arguments is preferred.
    auto exact = std::make_shared<MD>(MD{"m", {{.name = "a"}}, ""});
    CHECK(registry->insert(exact));
    CHECK(registry->find("m", 1) == exact);
    CHECK(registry->find("m", 2) == defaults);
  }
  SECTION("Arities may be split between child and parent") {
    auto parent = std::make_shared<MR>();
    auto child = std::make_shared<MR>(parent);
    auto one = std::make_shared<MD>(MD{"call", {{.name = "sym"}}, "one"});
    auto two = std::make_shared<MD>(MD{"call", {{.name = "rd"}, {.name = "sym"}}, "two"});
    CHECK(parent->insert(one));
    CHECK(child->insert(two));
    CHECK(child->find("call", 1) == one);
    CHECK(child->find("call", 2) == two);
    CHECK(child->find("call", 1, SM::Local) == nullptr);
  }
  SECTION("Purging a macro always hides parent definitions") {
    auto parent = std::make_shared<MR>();
    auto child = std::make_shared<MR>(parent);
    CHECK(parent->insert(std::make_shared<MD>(MD{"call", {{.name = "sym"}}, "one"})));
    CHECK(parent->insert(std::make_shared<MD>(MD{"call", {{.name = "rd"}, {.name = "sym"}}, "two"})));
    child->purge("call");
    CHECK(child->find("call", 1) == nullptr);
    CHECK(child->find("call", 2) == nullptr);
    auto redefined = std::make_shared<MD>(MD{"call", {{.name = "sym"}}, "redefined"});
    CHECK(child->insert(redefined));
    CHECK(child->find("call", 1) == redefined);
    // The parent's two-argument definition remains hidden.
    CHECK(child->find("call", 2) == nullptr);
    CHECK(child->contains("call"));
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
