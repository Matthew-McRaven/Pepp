#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>

#include <array>
#include <catch.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <string>
using namespace std::string_literals;
namespace fs = std::filesystem;

#include "svgdom/SvgCommentElement.hpp"
#include "svgdom/SvgDocument.hpp"
#include "svgdom/SvgRectElement.hpp"
#include "svgdom/SvgUseElement.hpp"
//	private classes
#include "svgdom/Timer.h"

TEST_CASE("Create/add svg elements", "[scope:core][scope:core.svgdom][kind:unit]") {
  //  Must use QT to copy files from resources.
  //  All files in temp directory are deleted on completion of test.
  QTemporaryDir dir;
  REQUIRE(QDir(dir.path()).mkdir("svgs"));
  auto file = dir.path().toStdString() + "/svgs/new.svg"s;

  SvgDocument doc1{};

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
  svg1.classList().add("className1");

  auto *redRect = svg1.createElementAs<SvgRectElement>(SvgType::Type::SvgRectElement);
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

  SvgDocument doc2{};
  doc2.open(file);
  SvgUnitValue tempValue;

  auto &svg2 = doc2.documentElement();
  CHECK(svg2.elementType() == SvgType::Type::SvgSvgElement);
  CHECK(svg2.xmlName() == "svg"s);
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
  CHECK(svg2.classList().contains("className1"s));

  //  Reuse rectangle pointer without data
  redRect = nullptr;
  redRect = static_cast<SvgRectElement *>(doc2.getElementById("red"s));
  REQUIRE(redRect != nullptr);
  if (redRect) {
    CHECK(redRect->id() == "red"s);
    CHECK(redRect->elementType() == SvgType::Type::SvgRectElement);
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
}

TEST_CASE("Create library file", "[scope:core][scope:core.svgdom][kind:unit]") {

  //  Must use QT to copy files from resources.
  //  All files in temp directory are deleted on completion of test.
  QTemporaryDir dir;
  REQUIRE(QDir(dir.path()).mkdir("svgs"));
  for (QDirIterator i(":/", QDirIterator::Subdirectories); i.hasNext();) {
    if (auto file = QFileInfo(i.next()); file.isFile()) {
      auto temp = dir.filePath("svgs/" + file.fileName());
      // std::cout << "File: "s << file.absoluteFilePath().toStdString() << ". Copy: "s << temp.toStdString() <<
      // std::endl;
      REQUIRE(QFile::copy(file.absoluteFilePath(), temp));
    }
  }

  std::string path = dir.path().toStdString() + "/svgs/{}.svg";

  std::array<std::string, 6> srcNames{"and", "inverter", "nand", "nor", "or", "xor"};
  std::vector<std::string> srcFiles;
  for (auto &srcName : srcNames) {
    srcFiles.push_back(fmt::format(fmt::runtime(path), srcName));
  }

  Timer t;
  t.start();
  SvgDocument doc1{};
  auto &svg = doc1.documentElement();

  auto *library = doc1.documentElement().createElement(SvgType::Type::SvgDefsElement);
  auto *gElement = doc1.documentElement().createElement(SvgType::Type::SvgGElement);

  REQUIRE(library != nullptr);
  REQUIRE(gElement != nullptr);

  for (int i = 0; i < srcFiles.size(); ++i) {
    SvgDocument d(srcFiles.at(i));

    auto *child = d.getElementById(srcNames.at(i));
    REQUIRE(child != nullptr);
    if (child) {
      library->appendChild(child);
    } else {
      std::cout << "Element ID not found: "s << srcNames.at(i) << std::endl;
    }

    auto *use = gElement->createElementAs<SvgUseElement>(SvgType::Type::SvgUseElement);
    REQUIRE(use != nullptr);
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

  auto file = dir.path().toStdString() + "/svgs/library.svg"s;

  doc1.saveAs(file);
  t.finish();
  REQUIRE(fs::exists(file));

  // std::cout << "Create library file: "s << t.elapsedTime() << std::endl;
}
