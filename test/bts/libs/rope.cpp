#include "bts/libs/rope.hpp"
#include <catch/catch.hpp>
#include <sstream>
#include <utility>

using namespace pepp::bts;
namespace {
std::string str1 = "This_is_a_test.";
std::string str2 = "Here is a much longer string for testing!";
std::string paragraph1 =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas sapien diam, maximus a mauris sed, posuere "
    "tincidunt tellus. Morbi sapien enim, vehicula sed imperdiet vel, pharetra vel lorem. Nullam pharetra justo ac "
    "elit varius, ut accumsan nisl eleifend. Mauris in condimentum augue. In consequat justo nunc, sit amet "
    "efficitur "
    "orci scelerisque at. Suspendisse ac ullamcorper urna, eget tincidunt risus. Suspendisse cursus nisl et volutpat "
    "ultrices. Integer posuere, diam vel tempus egestas, nisl leo tincidunt metus, nec semper risus nisl sit amet "
    "tortor. Morbi blandit sem sed nisi facilisis condimentum. Cras lacinia aliquet erat, nec finibus magna. "
    "Curabitur "
    "efficitur ante vitae efficitur vestibulum. Nam a accumsan urna, vitae consectetur lorem. Proin rutrum ultrices "
    "sapien ac tincidunt. Phasellus semper vel leo quis semper.";

// explode function for use in testing
// usage: auto v = explode("hello world foo bar", ' ');
std::vector<rope *> explode(const std::string &str, char delim) {
  std::vector<rope *> result;
  std::istringstream iss(str);
  for (std::string token; std::getline(iss, token, delim);) {
    rope *r = new rope(token);
    result.push_back(std::move(r));
  }
  return result;
}
// recover memory allocated via the explode function above
void reapExploded(std::vector<rope *> &v) {
  for (std::vector<rope *>::iterator iter = v.begin(); iter != v.end(); iter++) {
    delete *iter;
  }
  v.clear();
}
} // namespace

TEST_CASE("Test rope type for incremental ELF string sections", "[scope:elf][kind:unit][arch:*]") {
  SECTION("construction") {
    // test default constructor - produces a rope representing empty string
    rope r = rope();
    CHECK("" == r.toString());

    // test string constructor
    rope r2 = rope(str1);
    CHECK(str1 == r2.toString());

    // test copy constructor
    rope r3 = r2;
    CHECK(r2.toString() == r3.toString());
  }
  SECTION("assignment") {
    rope r;
    rope rTest = rope(str1);
    rope rParagraph = rope(paragraph1);

    r = rTest;
    CHECK(rTest.toString() == r.toString());

    // test self-assignment
    rTest = rTest;
    CHECK(rTest.toString() == rTest.toString());

    // test multiple assignment
    r = rTest = rParagraph;
    CHECK(paragraph1 == rTest.toString());
    CHECK(paragraph1 == r.toString());
  }
  SECTION("string ==") {
    rope rEmpty = rope();
    rope r1 = rope(str1);
    rope r1_Mutated = rope(str1);

    CHECK(rEmpty != r1);
    CHECK(r1 == r1_Mutated);

    r1_Mutated.rdelete(4, 1);
    r1_Mutated.rdelete(6, 1);
    r1_Mutated.rdelete(7, 1);

    rope tmpRope = rope("Thisisatest.");
    CHECK(r1_Mutated == tmpRope);

    r1_Mutated.insert(4, "_");
    r1_Mutated.insert(7, "_");
    r1_Mutated.insert(9, "_");
    CHECK(r1_Mutated == r1);
  }
  SECTION("at") {
    // test 'at' function
    rope r = rope(str1);
    CHECK('T' == r.at(0));
    CHECK('a' == r.at(8));

    r = rope(str2);
    CHECK('m' == r.at(10));
    CHECK_THROWS_AS(r.at(102), std::invalid_argument);
  }
  SECTION("length") {
    rope rEmpty = rope();
    rope r1 = rope(str1);
    rope r2 = rope(str2);

    CHECK(0 == rEmpty.length());
    CHECK(15 == r1.length());
    CHECK(41 == r2.length());
  }
  SECTION("insert") {
    rope rEmpty = rope();
    rope r1 = rope(str1);

    // test out-of-range indices
    CHECK_THROWS_AS(rEmpty.insert(1, "text"), std::invalid_argument);
    CHECK_THROWS_AS(r1.insert(120, ""), std::invalid_argument);

    // append string to an empty rope
    rEmpty.insert(0, "");
    CHECK("" == rEmpty.toString());
    rEmpty.insert(0, str1);
    CHECK(str1 == rEmpty.toString());

    // insert a non-empty string at beginning of a rope
    r1.insert(0, "Hello ");
    CHECK("Hello This_is_a_test." == r1.toString());
    // insert a non-empty string at end of a rope
    r1.insert(r1.length(), " Bye!");
    CHECK("Hello This_is_a_test. Bye!" == r1.toString());
    // insert a non-empty string at middle of a rope
    r1.insert(r1.length() - 1, " (more text)");
    CHECK("Hello This_is_a_test. Bye (more text)!" == r1.toString());

    rEmpty = rope();
    r1 = rope(str1);
    rope rHello = rope("Hello ");

    // append rope to an empty string (self-insertion)
    rEmpty.insert(0, rEmpty);
    CHECK("" == rEmpty.toString());
    rEmpty.insert(0, r1);
    CHECK(str1 == rEmpty.toString());

    // insert a non-empty rope into a non-empty rope
    r1.insert(0, rHello);
    CHECK("Hello This_is_a_test." == r1.toString());
  }
  SECTION("append") {
    // test concatenation of ropes representing non-empty strings
    rope r1 = rope(str1);
    rope r2 = rope(str2);
    r1.append(r2);
    CHECK("This_is_a_test.Here is a much longer string for testing!" == r1.toString());

    // test concatenation of two ropes both representing the empty string
    rope rEmpty = rope();
    rEmpty.append(rEmpty);
    CHECK("" == rEmpty.toString());

    // test concatenation of 'empty' rope with a rope representing a non-empty string
    r2.append(rEmpty);
    CHECK(str2 == r2.toString());
  }
  SECTION("at plus length plus append") {
    rope rEmpty = rope();
    rope r1 = rope(str1);
    rope r2 = rope(str2);

    // concatenate empty string and non-empty string
    rEmpty.append(r1);
    CHECK('T' == rEmpty.at(0));
    CHECK('a' == rEmpty.at(8));
    CHECK(15 == rEmpty.length());

    rEmpty.append(r2);
    CHECK('T' == rEmpty.at(0));
    CHECK('!' == rEmpty.at(55));
    CHECK(56 == rEmpty.length());
  }
  SECTION("substring") {
    rope rEmpty = rope();
    rope r1 = rope(str1);
    rope rParagraph = rope(paragraph1);

    // test out-of-range substring indices
    CHECK_THROWS_AS(rEmpty.substring(0, 1), std::invalid_argument);
    CHECK_THROWS_AS(rEmpty.substring(1, 0), std::invalid_argument);
    CHECK_THROWS_AS(r1.substring(1, 120), std::invalid_argument);

    // test empty string return conditions
    CHECK("" == rEmpty.substring(0, 0));
    CHECK("" == r1.substring(4, 0));

    // test non-zero substring length for valid substrings
    CHECK("This" == r1.substring(0, 4));
    CHECK("test." == r1.substring(10, 5));
    CHECK(" elit. Maecenas sapien diam, maximus a mauris sed," == rParagraph.substring(50, 50));
  }
  SECTION("delete") {
    rope rEmpty = rope();
    rope r1 = rope(str1);
    rope r2 = rope("Hello This_is_a_test. Bye (more text)!");

    // test out-of-range indices
    CHECK_THROWS_AS(rEmpty.rdelete(1, 0), std::invalid_argument);
    CHECK_THROWS_AS(r1.rdelete(120, 1), std::invalid_argument);

    // test invalid length parameters
    CHECK_THROWS_AS(rEmpty.rdelete(0, 1), std::invalid_argument);
    CHECK_THROWS_AS(r1.rdelete(9, 14), std::invalid_argument);

    // delete nothing from empty string
    rEmpty.rdelete(0, 0);
    CHECK("" == rEmpty.toString());

    // delete nothing from non-empty string
    r1.rdelete(4, 0);
    CHECK(str1 == r1.toString());

    size_t r2_len = r2.length();
    // delete from middle of string
    r2.rdelete(r2_len - 13, 12);
    CHECK("Hello This_is_a_test. Bye!" == r2.toString());

    r2_len = r2.length();
    // delete from end of string
    r2.rdelete(r2_len - 5, 5);
    CHECK("Hello This_is_a_test." == r2.toString());

    r2_len = r2.length();
    // delete from beginning of string
    r2.rdelete(0, 6);
    CHECK(str1 == r2.toString());

    // delete an entire string
    r2.rdelete(0, r2.length());
    CHECK("" == r2.toString());
  }
  SECTION("balance") {
    rope rF = rope("f");
    rope rE = rope("e");
    rope rD = rope("d");
    rope rC = rope("c");
    rope rB = rope("b");
    rope rA = rope("a");

    CHECK(rF.isBalanced());

    rope r1 = rope(str1);
    CHECK(r1.isBalanced());

    rF.insert(0, rE);
    CHECK(!rF.isBalanced());

    rF.insert(0, rD);
    rF.insert(0, rC);
    rF.insert(0, rB);
    rF.insert(0, rA);

    CHECK(!rF.isBalanced());

    rF.balance();

    CHECK(rF.isBalanced());

    CHECK("abcdef" == rF.toString());
  }
  SECTION("build and balance") {
    std::vector<rope *> exploded = explode(paragraph1, ' ');

    rope *rParagraph = exploded[0];
    size_t tmpLen;
    for (std::vector<rope *>::iterator iter = ++exploded.begin(); iter != exploded.end(); iter++) {
      tmpLen = rParagraph->length();
      rParagraph->insert(tmpLen, " ");
      rParagraph->insert(tmpLen + 1, **iter);
    }
    CHECK(paragraph1 == rParagraph->toString()); // compare to original text
    CHECK(!rParagraph->isBalanced());

    rParagraph->balance();
    CHECK(rParagraph->isBalanced());

    reapExploded(exploded);
  }
}
