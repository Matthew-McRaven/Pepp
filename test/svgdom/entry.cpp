#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>

#include <array>
#include <catch.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <typeindex>
#include <typeinfo>
using namespace std::string_literals;
namespace fs = std::filesystem;

#include "svgdom/SvgCommentElement.hpp"
#include "svgdom/SvgDocument.hpp"
#include "svgdom/SvgRectElement.hpp"
#include "svgdom/SvgUseElement.hpp"
//	private classes
#include "svgdom/Timer.h"

// Dummy test case to ensure that build doesn't fail due to lack of test cases.
/*TEST_CASE("Integration tests for svgdom", "[scope:core][scope:core.svgdom][kind:unit]") {
  // You should see a list of files ending with .svg in console output.
  // You should be able to read them into a QString, then convert it to an std::string.
  for (QDirIterator i(":/", QDirIterator::Subdirectories); i.hasNext();)
    if (auto f = QFileInfo(i.next()); f.isFile())
      std::cout << "File: "s << f.absoluteFilePath().toStdString() << std::endl;

  CHECK(true); // Don't let TEST_CASE braces collapse onto one line or Mac CI fails.
}*/

// Dummy test case to ensure that build doesn't fail due to lack of test cases.
TEST_CASE("Test opened and copy file", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Test that xml from one file can be copied to another file without error

  //  Must use QT to copy files from resources.
  //  All files in temp directory are deleted on completion of test.
  QTemporaryDir dir;
  REQUIRE(QDir(dir.path()).mkdir("svgs"));
  for (QDirIterator i(":/", QDirIterator::Subdirectories); i.hasNext();) {
    if (auto file = QFileInfo(i.next()); file.isFile()) {
      auto temp = dir.filePath("svgs/" + file.fileName());
      std::cout << "File: "s << file.absoluteFilePath().toStdString() << ". Copy: "s << temp.toStdString() << std::endl;
      REQUIRE(QFile::copy(file.absoluteFilePath(), temp));
    }
  }

  //  Rest of testing uses C++ 23 standard
  auto path = dir.path().toStdString() + "/svgs/"s;
  auto fileName = path + "sample.svg"s;
  auto copy = path + "copy.svg"s;
  auto output = path + "output.svg"s;

  //  Make sure file exists
  REQUIRE(fs::exists(fileName));

  Timer t;
  t.start();
  Document doc1{};
  doc1.open(fileName);
  REQUIRE(doc1.fileSize() > 0);

  //  First just copy existing document
  doc1.saveAs(copy);
  REQUIRE(fs::exists(copy));
  auto copySize = fs::file_size(copy);
  CHECK(doc1.fileSize() == copySize);

  //  Create second document from XML
  Document doc2{};
  doc2.fromXml(doc1.toXml());
  doc2.saveAs(output);
  t.finish();
  std::cout << "Create/copy to second file: " << t.elapsedTime() << std::endl;
  std::cout << "Doc1 size: " << doc1.fileSize() << ". Doc2 size: " << doc2.fileSize() << std::endl;

  REQUIRE(fs::exists(output));
  CHECK(doc1.fileSize() == doc2.fileSize());
}

TEST_CASE("Create/add svg elements", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Must use QT to copy files from resources.
  //  All files in temp directory are deleted on completion of test.
  QTemporaryDir dir;
  REQUIRE(QDir(dir.path()).mkdir("svgs"));
  auto file = dir.path().toStdString() + "/svgs/new.svg"s;

  Timer t;
  t.start();
  Document doc1{};

  auto &svg1 = doc1.documentElement();
  svg1.viewBox().setHeight(4);
  svg1.viewBox().setWidth(5);
  svg1.viewBox().setX(-.1);
  svg1.viewBox().setY(-.2);
  svg1.setHeight("50.01%"s);
  svg1.setWidth("50.02%"s);
  svg1.setTitle("Title1 for document"s);
  svg1.setDesc("Description1 for document"s);
  svg1.setMetadata("Metadata1 for document"s);

  auto *redRect = static_cast<SvgRectElement *>(svg1.createElement(SvgType::Type::SvgRectElement));
  REQUIRE(redRect != nullptr);
  if (redRect) {
    redRect->setId("red"s);
    redRect->setDesc("Red Desc"s);
    redRect->setMetadata("Red Meta"s);
    redRect->setTitle("Red Title"s);
    redRect->setHeight(1); //  From SvgElement
    redRect->setWidth(2);  //  From SvgElement
    redRect->setRx(.2);    //  From SvgRectElement
    redRect->setRy(.3);    //  From SvgRectElement
    redRect->setX(.5);
    redRect->setY(.6);

  } else {
    std::cout << "Cannot create red rectangle: "s << std::endl;
  }

  doc1.saveAs(file);
  CHECK(fs::exists(file));
  CHECK(doc1.fileSize() > 0);

  Document doc2{};
  doc2.open(file);
  SvgUnitValue tempValue;

  auto &svg2 = doc2.documentElement();
  CHECK(svg2.viewBox().height().toString() == SvgUnitValue(4).toString());
  CHECK(svg2.viewBox().width().toString() == SvgUnitValue(5).toString());
  CHECK(svg2.viewBox().x().toString() == SvgUnitValue(-.1).toString());
  CHECK(svg2.viewBox().y().toString() == SvgUnitValue(-.2).toString());
  tempValue.fromString("50.01%"s);
  CHECK(svg2.height().toString() == tempValue.toString());
  tempValue.fromString("50.02%"s);
  CHECK(svg2.width().toString() == tempValue.toString());

  CHECK(svg2.title() == "Title1 for document"s);
  CHECK(svg2.desc() == "Description1 for document"s);
  CHECK(svg2.metadata() == "Metadata1 for document"s);

  //  Reuse rectangle pointer without data
  redRect = nullptr;
  redRect = static_cast<SvgRectElement *>(doc2.getElementById("red"s));
  REQUIRE(redRect != nullptr);
  if (redRect) {
    CHECK(redRect->id() == "red"s);
    CHECK(redRect->desc() == "Red Desc"s);
    CHECK(redRect->metadata() == "Red Meta"s);
    CHECK(redRect->title() == "Red Title"s);
    CHECK(redRect->height().toString() == SvgUnitValue(1).toString());
    CHECK(redRect->width().toString() == SvgUnitValue(2).toString());
    CHECK(redRect->rx().toString() == SvgUnitValue(.2).toString());
    CHECK(redRect->ry().toString() == SvgUnitValue(.3).toString());
    CHECK(redRect->x().toString() == SvgUnitValue(.5).toString());
    CHECK(redRect->y().toString() == SvgUnitValue(.6).toString());

  } else {
    std::cout << "Cannot find red rectangle: "s << std::endl;
  }

  t.finish();
  std::cout << "Save/testing time: " << t.elapsedTime() << std::endl;
}

//  Test that element can be found by Id and updated
int test4(std::string_view path, const std::string &name, const std::string &id) {
  auto file = fmt::format(fmt::runtime(path), name);
  std::cout << "Open file: "s << file << std::endl;
  Timer t;
  t.start();
  Document doc1{};
  doc1.open(file);

  Document doc2{};
  doc2.copyDocument(doc1);
  auto *base = doc2.getElementById(id);
  std::cout << "base is " << typeid(*base).name() << std::endl;
  auto *derived = static_cast<SvgRectElement *>(doc2.getElementById("red"s));
  //  Below didn't work. Get compile errors
  // auto *derived = doc2.getElementById("red"s)->derived();
  std::cout << "derived is " << typeid(*derived).name() << std::endl;
  if (derived) {
    derived->setDesc("Red Desc"s);
    derived->setMetadata("Red Meta"s);
    derived->setTitle("Red Title"s);
    derived->setWidth(2); //  From SvgElement
    derived->setRy(.25);  //  From SvgRectElement

  } else {
    std::cout << "Element Id not found: "s << id << std::endl;
  }

  doc2.saveAs("x:\\"s + name + "-test4.svg"s);
  t.finish();
  std::cout << "Test4: Elapsed open/modify rectangle. "s << t.elapsedTime() << std::endl << std::endl;
  return 0;
}

//  Test that file can be opened and saved without error
int test5(std::string_view path, const std::string &name, const std::string &id) {
  auto file = fmt::format(fmt::runtime(path), name);
  std::cout << "Test5: Copy/Add elements to file: "s << file << std::endl;
  Timer t;
  t.start();
  Document doc1{};
  doc1.open(file);

  auto *derived = static_cast<SvgRectElement *>(doc1.getElementById(id));
  std::cout << "derived is " << typeid(*derived).name() << std::endl;
  if (derived) {
    derived->setDesc("Red Desc"s);
    derived->setMetadata("Red Meta"s);
    derived->setTitle("Red Title"s);
    derived->setWidth(2); //  From SvgElement
    derived->setRy(.25);  //  From SvgRectElement

  } else {
    std::cout << "Element ID not found: "s << id << std::endl;
    return 1;
  }

  //  Add elements from doc1
  Document doc2{};
  doc2.documentElement().appendChild(derived);
  auto *child = doc1.documentElement().createElement(SvgType::Type::SvgDefsElement);
  doc2.documentElement().appendChild(child);
  doc2.documentElement().viewBox().setHeight(5);
  doc2.documentElement().viewBox().setWidth(6);
  t.finish();

  doc2.saveAs("x:\\"s + name + "-test5.svg"s);
  t.finish();

  std::cout << "Test5: Copy/Add elements to file. "s << t.elapsedTime() << std::endl << std::endl;
  return 0;
}

//  Create libraby file
int test6(std::string_view path, const std::string &name) {
  std::array<std::string, 6> srcNames{"and", "inverter", "nand", "nor", "or", "xor"};
  std::vector<std::string> srcFiles;
  for (auto &srcName : srcNames) {
    srcFiles.push_back(fmt::format(fmt::runtime(path), srcName));
  }
  std::cout << "Test6: Create library file: "s << name << std::endl;

  Timer t;
  t.start();
  Document doc1{};
  auto *library = doc1.documentElement().createElement(SvgType::Type::SvgDefsElement);
  auto *gElement = doc1.documentElement().createElement(SvgType::Type::SvgGElement);

  for (int i = 0; i < srcFiles.size(); ++i) {
    Document d(srcFiles.at(i));

    auto *child = d.getElementById(srcNames.at(i));
    if (child) {
      library->appendChild(child);
    } else {
      std::cout << "Element ID not found: "s << srcNames.at(i) << std::endl;
    }

    auto *temp = gElement->createElement(SvgType::Type::SvgUseElement);
    auto *use = static_cast<SvgUseElement *>(temp);
    if (use) {
      use->setHref(srcNames.at(i));
      use->setX((i % 3) * 60);
      use->setY(i < 3 ? 0 : 40);
    } else {
      std::cout << "Use Element not found: "s << srcNames.at(i) << std::endl;
    }
  }
  doc1.documentElement().setHeight("90%"s);
  doc1.documentElement().setWidth("90%"s);
  doc1.documentElement().viewBox().setHeight(72);
  doc1.documentElement().viewBox().setWidth(168);

  //  Test size
  //<rect x="0" y="0" width="168" height="72" fill="red" fill-opacity=".25" />
  doc1.saveAs(name + "-test6.svg"s);
  t.finish();
  std::cout << "Test6: Elapsed open/save file. "s << t.elapsedTime() << std::endl << std::endl;
  return 0;
}

/*
int main() {
  const std::string name = "aa_rect"s;
  const std::string path = "/Users/mmcraven/code/Pepp/bin/circuit/svg/{}.svg";

  std::cout << "Start testing"s << std::endl;
  Timer t;
  t.start();
  int failed = 0;
  // failed += test1(path, "USStates");
  // failed += test2(path, name);
  // failed += test3(path, name);
  // failed += test4(path, "aa_rect"s, "red"s);
  // failed += test5(path, "aa_rect"s, "red"s);
  failed += test6(path, "library"s);

  //  Works anim3.svg, USStates.svg (88k)
  // doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\aa_rect.svg");
  // doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\car.svg"); //  500k file
  // doc1.open("E:\\Projects\\MSProjects\\CPP\\svgdom\\svg\\USStates.svg"); // Works!

  return failed;
}*/
