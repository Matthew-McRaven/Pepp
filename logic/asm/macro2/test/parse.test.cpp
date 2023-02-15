#include "catch.hpp"

#include "macro/parse.hpp"
TEST_CASE("Test symbol values.") {
  SECTION("added spaces") {
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci 0"_qs)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci 	0"_qs)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci 0	"_qs)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci  0"_qs)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci  0  "_qs)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci	0	"_qs)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deco​0​"_qs)) ==
          false); // 0-width space before and after
    CHECK(std::get<0>(macro::analyze_macro_definition(u" @deci 0"_qs)) ==
          false); // Can't have whitespace before
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@ deci 0"_qs)) ==
          false); // Can't have whitespace between @ and name
  }
  SECTION("differnt arity") {
    auto _0ar = macro::analyze_macro_definition(u"@deci 0"_qs);
    auto _8ar = macro::analyze_macro_definition(u"@deco 8"_qs);
    CHECK(std::get<0>(_0ar));
    CHECK(std::get<1>(_0ar).toUtf8().toStdString() == "deci");
    CHECK(std::get<2>(_0ar) == 0);
    CHECK(std::get<0>(_8ar));
    CHECK(std::get<1>(_8ar).toUtf8().toStdString() == "deco");
    CHECK(std::get<2>(_8ar) == 8);
  }
  SECTION("reject comments") {
    CHECK(std::get<0>(macro::analyze_macro_definition("@deci 2 ;fail")) ==
          false);
  }
}
