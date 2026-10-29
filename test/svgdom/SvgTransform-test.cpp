#include <catch.hpp>

// #include <filesystem>
#include <iostream>
using namespace std::string_literals;
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

  //  Test string parsing
  test.fromString("rotate( -1.1,2.2 , -3.3) scale ( 6 7) skewX (+25.5 ) skewY(-450 ) translate (23.4 ,43.2 )"s);
  CHECK(test.rotateAngle() == -1.1);
  CHECK(test.rotateX() == 2.2);
  CHECK(test.rotateY() == -3.3);
  CHECK(test.scaleX() == 6);
  CHECK(test.scaleY() == 7);
  CHECK(test.skewX() == 25.5);
  CHECK(test.skewY() == -90); //  Clamped to 0 & -359
  CHECK(test.translateX() == 23.4);
  CHECK(test.translateY() == 43.2);
  CHECK(test.toString() ==
        "transform=\"rotate(-1.1 2.2 -3.3) scale(6 7) skewX(25.5) skewY(-90) translate(23.4 43.2)\""s);

  //  Change values
  test.setSkewX(0);        //  Removes SkewX
  test.setRotate(321);     //  Moves rotate to end
  test.setTranslate(0, 0); //  Removes Translate
  CHECK(test.toString() == "transform=\"scale(6 7) skewY(-90) rotate(321)\""s);
}

TEST_CASE("SvgTransform tests-parse transform from svg", "[scope:core][scope:core.svgdom.svgtransform4][kind:unit]") {
  //  Test that SvgTransform class perform correctly
  //  Test that transformations can be read from file

  // std::cout << "From SvgTransform" << std::endl;

  //  Test rotation
  SvgTransform test;
  test.fromString("rotate(25.1)"s);
  CHECK(test.rotateAngle() == 25.1);
  CHECK(test.rotateX() == 0);
  CHECK(test.rotateY() == 0);
  CHECK(test.toString() == "transform=\"rotate(25.1)\""s);
  test.fromString("rotate(-1.1, 2.2,-3.3)"s);
  CHECK(test.rotateAngle() == -1.1);
  CHECK(test.rotateX() == 2.2);
  CHECK(test.rotateY() == -3.3);
  CHECK(test.toString() == "transform=\"rotate(-1.1 2.2 -3.3)\""s);

  //  Test scale
  test.clear();
  test.fromString("scale( 5 )"s);
  CHECK(test.scaleX() == 5);
  CHECK(test.scaleY() == 0);
  CHECK(test.toString() == "transform=\"scale(5)\""s);
  test.clear();
  test.fromString("scale( 6, 7 )"s);
  CHECK(test.scaleX() == 6);
  CHECK(test.scaleY() == 7);
  CHECK(test.toString() == "transform=\"scale(6 7)\""s);

  //  Test skew
  test.clear();
  test.fromString("skewX (25.5)"s);
  CHECK(test.skewX() == 25.5);
  CHECK(test.toString() == "transform=\"skewX(25.5)\""s);
  test.clear();
  test.fromString("skewY (+24.5) "s);
  CHECK(test.skewY() == 24.5);
  CHECK(test.toString() == "transform=\"skewY(24.5)\""s);

  //  Test translate
  test.clear();
  test.fromString("translate ( 12.3 ) "s);
  CHECK(test.translateX() == 12.3);
  CHECK(test.translateY() == 0);
  CHECK(test.toString() == "transform=\"translate(12.3)\""s);
  test.clear();
  test.fromString("translate ( -23.4, 43.2 ) "s);
  CHECK(test.translateX() == -23.4);
  CHECK(test.translateY() == 43.2);
  CHECK(test.toString() == "transform=\"translate(-23.4 43.2)\""s);
}
