/// The MIT License (MIT)
/// Copyright (c) 2016 Peter Goldsborough
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.


#include "lru/internal/last-accessed.hpp"

#include <catch.hpp>
#include <iterator>
#include <string>
#include <unordered_map>

using namespace LRU::Internal;

// clang-format off
using Map = std::unordered_map<std::string, int>;
Map map = {
  {"one", 1},
  {"two", 2},
  {"three", 3}
};
// clang-format on

TEST_CASE("LastAccessedTest") {
  SECTION("IsAssignableFromConstAndNonConst") {
    auto front = map.find("one");

    LastAccessed<std::string, int> last_accessed(front->first, front->second);

    REQUIRE(last_accessed.key() == "one");
    REQUIRE(last_accessed.information() == 1);

    last_accessed = map.find("two");

    CHECK(last_accessed.key() == "two");
    CHECK(last_accessed.information() == 2);

    last_accessed = map.find("three");

    CHECK(last_accessed.key() == "three");
    CHECK(last_accessed.information() == 3);
  }
  SECTION("IsComparableWithConstAndNonConstIterators") {
    auto front = map.find("one");
    LastAccessed<std::string, int> last_accessed(front->first, front->second);

    // non-const
    CHECK(last_accessed == front);
    CHECK(front == last_accessed);

    CHECK(map.find("two") != last_accessed);
    CHECK(last_accessed != map.find("three"));

    // const
    Map::const_iterator const_front = map.find("one");
    CHECK(last_accessed == const_front);
    CHECK(const_front == last_accessed);

    Map::const_iterator iterator = map.find("two");
    CHECK(iterator != last_accessed);

    iterator = map.find("three");
    CHECK(last_accessed != iterator);
  }
  SECTION("IsComparableToConstAndNonConstKeys") {
    using namespace std::string_literals;

    std::string key = "forty-two";
    int information = 42;

    LastAccessed<std::string, int> last_accessed(key, information);

    CHECK(last_accessed == key);
    CHECK(key == last_accessed);

    CHECK(last_accessed == "forty-two"s);
    CHECK("forty-two"s == last_accessed);

    const std::string& key_const_reference = key;

    CHECK(key_const_reference == last_accessed);
    CHECK(last_accessed == key_const_reference);

    CHECK(last_accessed != "asdf"s);
    CHECK(last_accessed != "foo"s);
    CHECK(last_accessed != "forty-three"s);
  }
}
