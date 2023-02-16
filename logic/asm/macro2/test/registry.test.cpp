#include <catch.hpp>

#include "macro/macro.hpp"
#include "macro/registry.hpp"
TEST_CASE("macro::Registry") {
  SECTION("registers macros") {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body");
    auto registered = reg.registerMacro(macro::Type::Core, parsed);
    CHECK(registered != nullptr);
    CHECK(registered->contents() == parsed);
  }
  SECTION("can find macros by name") {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body");
    auto registered = reg.registerMacro(macro::Type::Core, parsed);
    CHECK(registered != nullptr);
    CHECK(registered->contents() == parsed);
    CHECK(reg.findMacro("alpha") == registered);
  }
  SECTION("reject macros with duplicate names") {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body");
    auto parsed2 = new macro::Parsed("alpha", 0, "body");
    auto registered = reg.registerMacro(macro::Type::Core, parsed);
    CHECK(registered != nullptr);
    CHECK(reg.registerMacro(macro::Type::Core, parsed2) == nullptr);
  }
  SECTION("distinguishes macro types") {
    macro::Registry reg;
    auto parsed = new macro::Parsed("alpha", 0, "body");
    auto parsed2 = new macro::Parsed("beta", 0, "body");
    CHECK(reg.registerMacro(macro::Type::Core, parsed) != nullptr);
    CHECK(reg.registerMacro(macro::Type::System, parsed2) != nullptr);
    CHECK(reg.findMacrosByType(macro::Type::Core).size() == 1);
    CHECK(reg.findMacrosByType(macro::Type::System).size() == 1);
    CHECK(reg.findMacrosByType(macro::Type::User).size() == 0);
  }
}
