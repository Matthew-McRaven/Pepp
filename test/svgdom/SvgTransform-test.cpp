#include <catch.hpp>

// #include <iostream>
using namespace std::string_literals;

// #include <filesystem>
// namespace fs = std::filesystem;

// #include <QFile>
// #include <QTemporaryDir>

#include "svgdom/SvgTransform.hpp"
// #include "svgdom/SvgDocument.hpp"

void checkClear(const SvgTransform &test);

TEST_CASE("SvgTransform tests-getter-setter", "[scope:core][scope:core.svgdom.svgtransform1][kind:unit]") {
  //  Test that SvgTransform class perform correctly
  //  Test getter/setters

  // std::cout << "From SvgTransform" << std::endl;
  SvgTransform test;
  checkClear(test);

  //  Test rotation
  test.setRotate(25);
  CHECK(test.type() == SvgTransform::TransformType::Rotate);
  CHECK(test.rotateAngle() == 25);
  CHECK(test.rotateX() == 0);
  CHECK(test.rotateY() == 0);
  test.setRotate(-1, 2, -3);
  CHECK(test.type() == SvgTransform::TransformType::Rotate);
  CHECK(test.rotateAngle() == -1);
  CHECK(test.rotateX() == 2);
  CHECK(test.rotateY() == -3);

  //  Values over 360 are clamped to 0 and 359
  test.setRotate(1200);
  CHECK(test.rotateAngle() == 120);

  //  Negative values over 360 are clamped to 0 and 359
  test.setRotate(-1200);
  CHECK(test.rotateAngle() == -120);

  //  Test scale
  test.setScale(5);
  CHECK(test.type() == SvgTransform::TransformType::Scale);
  CHECK(test.scaleX() == 5);
  CHECK(test.scaleY() == 0);
  test.setScale(6, 7);
  CHECK(test.type() == SvgTransform::TransformType::Scale);
  CHECK(test.scaleX() == 6);
  CHECK(test.scaleY() == 7);

  //  Test skew
  test.setSkewX(25.5);
  CHECK(test.type() == SvgTransform::TransformType::SkewX);
  CHECK(test.skewX() == 25.5);
  //  Values over 360 are clamped to 0 and 359
  test.setSkewX(1200);
  CHECK(test.type() == SvgTransform::TransformType::SkewX);
  CHECK(test.skewX() == 120);
  test.setSkewY(24.5);
  CHECK(test.type() == SvgTransform::TransformType::SkewY);
  CHECK(test.skewY() == 24.5);
  //  Negative values over 360 are clamped to 0 and 359
  test.setSkewY(-450);
  CHECK(test.type() == SvgTransform::TransformType::SkewY);
  CHECK(test.skewY() == -90);

  //  Test translate
  test.setTranslate(12.3);
  CHECK(test.type() == SvgTransform::TransformType::Translate);
  CHECK(test.translateX() == 12.3);
  CHECK(test.translateY() == 0);
  test.setTranslate(23.4, 43.2);
  CHECK(test.type() == SvgTransform::TransformType::Translate);
  CHECK(test.translateX() == 23.4);
  CHECK(test.translateY() == 43.2);

  test.clear();
  checkClear(test);
}

void checkClear(const SvgTransform &test) {
  CHECK(test.type() == SvgTransform::TransformType::Unknown);
  CHECK(test.rotateAngle() == 0);
  CHECK(test.rotateX() == 0);
  CHECK(test.rotateY() == 0);
  CHECK(test.scaleX() == 1.0);
  CHECK(test.scaleY() == 1.0);
  CHECK(test.skewX() == 0);
  CHECK(test.skewY() == 0);
  CHECK(test.translateX() == 0);
  CHECK(test.translateY() == 0);
}

TEST_CASE("SvgTransform tests-toString", "[scope:core][scope:core.svgdom.svgtransform2][kind:unit]") {
  //  Test that SvgTransform class perform correctly
  //  Test toString()

  // std::cout << "From SvgTransform" << std::endl;
  //  Test rotation
  {
    SvgTransform test;
    CHECK(test.toString() == ""s);

    test.setRotate(25.1);
    CHECK(test.toString() == "rotate(25.1)"s);
    test.setRotate(-1.1, 2.2, -3.3);
    CHECK(test.toString() == "rotate(-1.1 2.2 -3.3)"s);
    //  Resetting to default value removes element from output
    test.setRotate(0);
    CHECK(test.toString() == ""s);
    test.setRotate(-1.1, 2.2, -3.3);
    test.setRotate(0, 0, 0);
    CHECK(test.toString() == ""s);
  }

  //  Test scale
  {
    SvgTransform test;
    test.setScale(5);
    CHECK(test.toString() == "scale(5)"s);
    test.setScale(6, 7);
    CHECK(test.toString() == "scale(6 7)"s);
    //  Resetting to default value removes element from output
    test.setScale(1, 0);
    CHECK(test.toString() == ""s);
  }

  //  Test skew X
  {
    SvgTransform test;
    test.setSkewX(25.5);
    CHECK(test.toString() == "skewX(25.5)"s);
    //  Values over 360 are clamped to 0 and 359
    test.setSkewX(1200);
    CHECK(test.toString() == "skewX(120)"s);
    //  Resetting to default value removes element from output
    test.setSkewX(0);
    CHECK(test.toString() == ""s);
  }

  //  Test skew Y
  {
    SvgTransform test;
    test.setSkewY(24.5);
    CHECK(test.toString() == "skewY(24.5)"s);
    //  Negative values over 360 are clamped to 0 and 359
    test.setSkewY(-450);
    CHECK(test.toString() == "skewY(-90)"s);
    //  Resetting to default value removes element from output
    test.setSkewY(0);
    CHECK(test.toString() == ""s);
  }

  //  Test translate
  {
    SvgTransform test;
    test.setTranslate(12.3);
    CHECK(test.toString() == "translate(12.3)"s);
    test.setTranslate(23.4, 43.2);
    CHECK(test.toString() == "translate(23.4 43.2)"s);
    //  Resetting to default value removes element from output
    test.setTranslate(0, 0);
    CHECK(test.toString() == ""s);
  }
}

TEST_CASE("SvgTransformList tests", "[scope:core][scope:core.svgdom.svgtransformlist1][kind:unit]") {
  //  Test that SvgTransformList class perform correctly

  // std::cout << "From SvgTransformList" << std::endl;
  //  Test rotation

  SvgTransformList list;
  CHECK(list.size() == 0);

  {
    auto &test = list.appendItem();

    test.setRotate(25.1);
    CHECK(list.toString() == "transform=\"rotate(25.1)\""s);
    test.setRotate(-1.1, 2.2, -3.3);
    CHECK(list.toString() == "transform=\"rotate(-1.1 2.2 -3.3)\""s);
    //  Resetting to default value removes element from output
    test.setRotate(0);
    CHECK(list.toString() == ""s);
  }

  //  Test scale
  list.clear();
  {
    auto &test = list.appendItem();
    test.setScale(5);
    CHECK(list.toString() == "transform=\"scale(5)\""s);
    test.setScale(6, 7);
    CHECK(list.toString() == "transform=\"scale(6 7)\""s);
    //  Resetting to default value removes element from output
    test.setScale(1, 0);
    CHECK(list.toString() == ""s);
  }

  //  Test skew X
  list.clear();
  {
    auto &test = list.appendItem();
    test.setSkewX(25.5);
    CHECK(list.toString() == "transform=\"skewX(25.5)\""s);
    //  Values over 360 are clamped to 0 and 359
    test.setSkewX(1200);
    CHECK(list.toString() == "transform=\"skewX(120)\""s);
    //  Resetting to default value removes element from output
    test.setSkewX(0);
    CHECK(list.toString() == ""s);
  }

  //  Test skew Y
  list.clear();
  {
    auto &test = list.appendItem();
    test.setSkewY(24.5);
    CHECK(list.toString() == "transform=\"skewY(24.5)\""s);
    //  Negative values over 360 are clamped to 0 and 359
    test.setSkewY(-450);
    CHECK(list.toString() == "transform=\"skewY(-90)\""s);
    //  Resetting to default value removes element from output
    test.setSkewY(0);
    CHECK(list.toString() == ""s);
  }

  //  Test translate
  list.clear();
  {
    auto &test = list.appendItem();
    test.setTranslate(12.3);
    CHECK(list.toString() == "transform=\"translate(12.3)\""s);
    test.setTranslate(23.4, 43.2);
    CHECK(list.toString() == "transform=\"translate(23.4 43.2)\""s);
    //  Resetting to default value removes element from output
    test.setTranslate(0, 0);
    CHECK(list.toString() == ""s);
  }
}

TEST_CASE("SvgTransformList tests-parse transform from svg",
          "[scope:core][scope:core.svgdom.svgtransformlist2][kind:unit]") {
  //  Test that SvgTransformList class perform correctly
  //  Test that transformations can be read from file

  // std::cout << "From SvgTransformList" << std::endl;

  SvgTransformList list;
  CHECK(list.size() == 0);

  //  Test rotation
  {
    list.fromString("rotate(25.1)"s);
    auto test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->rotateAngle() == 25.1);
    CHECK(test->rotateX() == 0);
    CHECK(test->rotateY() == 0);
    CHECK(list.toString() == "transform=\"rotate(25.1)\""s);

    list.clear();
    list.fromString("rotate(-1.1, 2.2,-3.3)"s);
    test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->rotateAngle() == -1.1);
    CHECK(test->rotateX() == 2.2);
    CHECK(test->rotateY() == -3.3);
    CHECK(list.toString() == "transform=\"rotate(-1.1 2.2 -3.3)\""s);
  }

  //  Test scale
  list.clear();
  {
    list.fromString("scale( 5 )"s);
    auto test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->scaleX() == 5);
    CHECK(test->scaleY() == 0);
    CHECK(list.toString() == "transform=\"scale(5)\""s);

    list.clear();
    list.fromString("scale( 6, 7 )"s);
    test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->scaleX() == 6);
    CHECK(test->scaleY() == 7);
    CHECK(list.toString() == "transform=\"scale(6 7)\""s);
  }

  //  Test skew
  list.clear();
  {
    list.fromString("skewX (25.5)"s);
    auto test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->skewX() == 25.5);
    CHECK(list.toString() == "transform=\"skewX(25.5)\""s);

    list.clear();
    list.fromString("skewY (+24.5) "s);
    test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->skewY() == 24.5);
    CHECK(list.toString() == "transform=\"skewY(24.5)\""s);
  }

  //  Test translate
  list.clear();
  {
    list.fromString("translate ( 12.3 ) "s);
    auto test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->translateX() == 12.3);
    CHECK(test->translateY() == 0);
    CHECK(list.toString() == "transform=\"translate(12.3)\""s);

    list.clear();
    list.fromString("translate ( -23.4, 43.2 ) "s);
    test = list.getItem(0);
    REQUIRE(test != nullptr);
    CHECK(test->translateX() == -23.4);
    CHECK(test->translateY() == 43.2);
    CHECK(list.toString() == "transform=\"translate(-23.4 43.2)\""s);
  }
}

TEST_CASE("SvgTransform tests-multiple transformations",
          "[scope:core][scope:core.svgdom.svgtransformlist3][kind:unit]") {
  //  Test that SvgTransform class perform correctly
  //  Test multiple transformations

  // std::cout << "From SvgTransform" << std::endl;

  SvgTransformList list1;

  list1.appendItem().setRotate(-1.1, 2.2, -3.3);
  list1.appendItem().setScale(6, 7);
  list1.appendItem().setSkewX(25.5);
  list1.appendItem().setSkewY(-450);
  list1.appendItem().setTranslate(23.4, 43.2);
  CHECK(list1.toString() ==
        "transform=\"rotate(-1.1 2.2 -3.3) scale(6 7) skewX(25.5) skewY(-90) translate(23.4 43.2)\""s);

  //  Test string parsing
  SvgTransformList list2;
  list2.fromString("rotate( -1.1,2.2 , -3.3) scale ( 6 7) skewX (+25.5 ) skewY(-450 ) translate (23.4 ,43.2 )"s);
  {
    auto test2 = list2.getItem(0);
    REQUIRE(test2 != nullptr);
    CHECK(test2->rotateAngle() == -1.1);
    CHECK(test2->rotateX() == 2.2);
    CHECK(test2->rotateY() == -3.3);

    test2 = list2.getItem(1);
    REQUIRE(test2 != nullptr);
    CHECK(test2->scaleX() == 6);
    CHECK(test2->scaleY() == 7);

    test2 = list2.getItem(2);
    REQUIRE(test2 != nullptr);
    CHECK(test2->skewX() == 25.5);

    test2 = list2.getItem(3);
    REQUIRE(test2 != nullptr);
    CHECK(test2->skewY() == -90); //  Clamped to 0 & -359

    test2 = list2.getItem(4);
    REQUIRE(test2 != nullptr);
    CHECK(test2->translateX() == 23.4);
    CHECK(test2->translateY() == 43.2);

    // Error condition
    test2 = list2.getItem(5);
    CHECK(test2 == nullptr);

    CHECK(list2.toString() ==
          "transform=\"rotate(-1.1 2.2 -3.3) scale(6 7) skewX(25.5) skewY(-90) translate(23.4 43.2)\""s);
  }
  //  Change values
  list2.removeItem(2);                      //  Removes SkewX
  list2.removeItem(3);                      //  Remove Translate
  list2.getItem(0)->setTranslate(123, 456); //  Update first item to translate
  CHECK(list2.toString() == "transform=\"translate(123 456) scale(6 7) skewY(-90)\""s);
}
