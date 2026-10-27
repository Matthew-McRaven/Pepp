#include <catch.hpp>

#include <iostream>
using namespace std::string_literals;

#include "svgdom/SvgTransform.hpp"

void checkClear(const SvgTransform &test);

TEST_CASE("SvgTransform tests-getter-setter", "[scope:core][scope:core.svgdom.svgtransform1][kind:unit]") {
  //  Test that SvgTransform class perform correctly
  //  Test getter/setters

  // std::cout << "From SvgTransform" << std::endl;
  SvgTransform test;
  checkClear(test);

  //  Test rotation
  test.setRotate(25);
  CHECK(test.rotateAngle() == 25);
  CHECK(test.rotateX() == 0);
  CHECK(test.rotateY() == 0);
  test.setRotate(-1, 2, -3);
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
  CHECK(test.scaleX() == 5);
  CHECK(test.scaleY() == 0);
  test.setScale(6, 7);
  CHECK(test.scaleX() == 6);
  CHECK(test.scaleY() == 7);

  //  Test skew
  test.setSkewX(25.5);
  CHECK(test.skewX() == 25.5);
  //  Values over 360 are clamped to 0 and 359
  test.setSkewX(1200);
  CHECK(test.skewX() == 120);
  test.setSkewY(24.5);
  CHECK(test.skewY() == 24.5);
  //  Negative values over 360 are clamped to 0 and 359
  test.setSkewY(-450);
  CHECK(test.skewY() == -90);

  //  Test translate
  test.setTranslate(12.3);
  CHECK(test.translateX() == 12.3);
  CHECK(test.translateY() == 0);
  test.setTranslate(23.4, 43.2);
  CHECK(test.translateX() == 23.4);
  CHECK(test.translateY() == 43.2);

  test.clear();
  checkClear(test);
}

void checkClear(const SvgTransform &test) {
  CHECK(test.rotateAngle() == 0);
  CHECK(test.rotateX() == 0);
  CHECK(test.rotateY() == 0);
  CHECK(test.scaleX() == 1);
  CHECK(test.scaleY() == 0);
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
    CHECK(test.toString() == "transform=\"rotate(25.1)\""s);
    test.setRotate(-1.1, 2.2, -3.3);
    CHECK(test.toString() == "transform=\"rotate(-1.1 2.2 -3.3)\""s);
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
    CHECK(test.toString() == "transform=\"scale(5)\""s);
    test.setScale(6, 7);
    CHECK(test.toString() == "transform=\"scale(6 7)\""s);
    //  Resetting to default value removes element from output
    test.setScale(1, 0);
    CHECK(test.toString() == ""s);
  }

  //  Test skew X
  {
    SvgTransform test;
    test.setSkewX(25.5);
    CHECK(test.toString() == "transform=\"skewX(25.5)\""s);
    //  Values over 360 are clamped to 0 and 359
    test.setSkewX(1200);
    CHECK(test.toString() == "transform=\"skewX(120)\""s);
    //  Resetting to default value removes element from output
    test.setSkewX(0);
    CHECK(test.toString() == ""s);
  }

  //  Test skew Y
  {
    SvgTransform test;
    test.setSkewY(24.5);
    CHECK(test.toString() == "transform=\"skewY(24.5)\""s);
    //  Negative values over 360 are clamped to 0 and 359
    test.setSkewY(-450);
    CHECK(test.toString() == "transform=\"skewY(-90)\""s);
    //  Resetting to default value removes element from output
    test.setSkewY(0);
    CHECK(test.toString() == ""s);
  }

  //  Test translate
  {
    SvgTransform test;
    test.setTranslate(12.3);
    CHECK(test.toString() == "transform=\"translate(12.3)\""s);
    test.setTranslate(23.4, 43.2);
    CHECK(test.toString() == "transform=\"translate(23.4 43.2)\""s);
    //  Resetting to default value removes element from output
    test.setTranslate(0, 0);
    CHECK(test.toString() == ""s);
  }
}

TEST_CASE("SvgTransform tests-multiple transformations", "[scope:core][scope:core.svgdom.svgtransform3][kind:unit]") {
  //  Test that SvgTransform class perform correctly
  //  Test multiple transformations

  // std::cout << "From SvgTransform" << std::endl;
  SvgTransform test;
  CHECK(test.toString() == ""s);

  test.setRotate(-1.1, 2.2, -3.3);
  test.setScale(6, 7);
  test.setSkewX(25.5);
  test.setSkewY(-450);
  test.setTranslate(23.4, 43.2);
  CHECK(test.toString() ==
        "transform=\"rotate(-1.1 2.2 -3.3) scale(6 7) skewX(25.5) skewY(-90) translate(23.4 43.2)\""s);

  //  Change values
  test.setSkewX(0);        //  Removes value
  test.setRotate(321);     //  Moves value
  test.setTranslate(0, 0); //  Removes value
  CHECK(test.toString() == "transform=\"scale(6 7) skewY(-90) rotate(321)\""s);
}
