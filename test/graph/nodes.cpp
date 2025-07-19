// Copied from CXXGraph/test/NodeTest.cpp to ensure that our build system properly clones the library
// Code is under the terms of that repo.
#include <catch.hpp>
#include "CXXGraph/CXXGraph.hpp"

// Smart pointers alias
template <typename T> using unique = std::unique_ptr<T>;
template <typename T> using shared = std::shared_ptr<T>;

using std::make_shared;
using std::make_unique;

// Struct for testing Node as a template of different items
struct testStruct {
  int testInt = 0;
  bool testBool = true;
  std::string testString = "abc";

  testStruct() = default;
  explicit testStruct(int a, bool b, const std::string &c) : testInt(a), testBool(b), testString(c) {};

  bool operator==(const testStruct &b) const {
    return (this->testInt == b.testInt && this->testBool == b.testBool && this->testString == b.testString);
  };
};

TEST_CASE("CXX Graph Test") {
  SECTION("String Constructor") {
    // Testing constructor and getters with different types of data
    int intTest = 42;
    float floatTest = 3.14f;
    double doubleTest = 3.1416;
    bool boolTest = true;
    char charTest = 'w';
    std::string stringTest = "myStr";
    std::vector<int> vectorTest = {1, 2, 3, 4};
    std::unordered_map<int, int> mapTest = {{1, 2}, {3, 4}, {5, 6}, {7, 8}};
    testStruct structTest(42, true, "abc");

    CXXGraph::Node<int> intNode("1", intTest);
    CXXGraph::Node<float> floatNode("2", floatTest);
    CXXGraph::Node<double> doubleNode("3", doubleTest);
    CXXGraph::Node<bool> boolNode("4", boolTest);
    CXXGraph::Node<char> charNode("5", charTest);
    CXXGraph::Node<std::string> stringNode("6", stringTest);
    CXXGraph::Node<std::vector<int>> vectorNode("7", vectorTest);
    CXXGraph::Node<std::unordered_map<int, int>> mapNode("8", mapTest);
    CXXGraph::Node<testStruct> structNode("9", structTest);

    CHECK(intNode.getUserId() == "1");
    CHECK(intNode.getData() == intTest);
    CHECK(floatNode.getUserId() == "2");
    CHECK(floatNode.getData() == floatTest);
    CHECK(doubleNode.getUserId() == "3");
    CHECK(doubleNode.getData() == doubleTest);
    CHECK(boolNode.getUserId() == "4");
    CHECK(boolNode.getData() == boolTest);
    CHECK(charNode.getUserId() == "5");
    CHECK(charNode.getData() == charTest);
    CHECK(stringNode.getUserId() == "6");
    CHECK(stringNode.getData() == stringTest);
    CHECK(vectorNode.getUserId() == "7");
    CHECK(vectorNode.getData() == vectorTest);
    CHECK(mapNode.getUserId() == "8");
    CHECK(mapNode.getData() == mapTest);
    CHECK(structNode.getUserId() == "9");
    CHECK(structNode.getData() == structTest);
  }
}
