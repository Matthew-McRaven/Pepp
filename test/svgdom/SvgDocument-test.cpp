#include <catch.hpp>

#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>

#include <filesystem>
#include <string>
// #include <iostream>
using namespace std::string_literals;
namespace fs = std::filesystem;

#include "svgdom/SvgDocument.hpp"
#include "svgdom/SvgElement.hpp"

TEST_CASE("SvgDocument tests-file operations", "[scope:core][scope:core.svgdom.svgdocument1][kind:unit]") {
  //  Test that SvgDocument class perform correctly

  // std::cout << "From SvgDocument" << std::endl;
  //  Must use QT to copy files from resources.
  //  All files in temp directory are deleted on completion of test.
  QTemporaryDir dir;
  REQUIRE(QDir(dir.path()).mkdir("svgs"));
  auto source = ":/svgs/sample.svg";
  auto target = dir.filePath("svgs/sample.svg");
  // std::cout << "Source: "s << source << " Target: "s << target.toStdString() << std::endl;
  REQUIRE(QFile::copy(source, target));
  REQUIRE(fs::exists(target.toStdString()));

  //  Rest of testing uses C++ 23 standard
  auto path = dir.path().toStdString() + "/svgs/"s;
  auto original = target.toStdString();
  auto copy = path + "copy.svg"s;
  auto copy2 = path + "copy2.svg"s;
  auto output = path + "output.svg"s;

  //  Make sure file exists
  REQUIRE(fs::exists(original));

  SvgDocument doc1{};
  REQUIRE(!doc1.exists());
  doc1.open(original);
  REQUIRE(doc1.fileSize() > 0);
  REQUIRE(doc1.exists());

  //  First just copy existing document
  doc1.saveAs(copy);
  REQUIRE(fs::exists(copy));
  auto copySize = fs::file_size(copy);
  CHECK(doc1.fileSize() == copySize);

  //  Create second document from XML
  SvgDocument doc2{};
  doc2.fromXml(doc1.toXml());
  doc2.saveAs(output);

  REQUIRE(fs::exists(output));
  CHECK(doc1.fileSize() == doc2.fileSize());

  //  Create second document from original
  SvgDocument doc3{};
  doc3.copyDocument(doc1);
  doc3.saveAs(copy2);

  REQUIRE(fs::exists(copy2));
  CHECK(doc1.fileSize() == doc3.fileSize());
}

TEST_CASE("SvgDocument tests-element operations", "[scope:core][scope:core.svgdom.svgdocument2][kind:unit]") {
  //  Test that SvgDocument class perform correctly

  // std::cout << "From SvgDocument" << std::endl;
  //  Must use QT to copy files from resources.
  //  All files in temp directory are deleted on completion of test.
  QTemporaryDir dir;
  REQUIRE(QDir(dir.path()).mkdir("svgs"));
  auto source = ":/svgs/sample.svg";
  auto target = dir.filePath("svgs/sample.svg");

  REQUIRE(QFile::copy(source, target));
  REQUIRE(fs::exists(target.toStdString()));

  //  Rest of testing uses C++ 23 standard
  auto path = dir.path().toStdString() + "/svgs/"s;
  auto original = target.toStdString();

  SvgDocument doc1{};
  CHECK(doc1.documentElement().elementType() == SvgType::Type::SvgSvgElement);
  auto *e = doc1.documentElement().createElementAs<SvgElement>(SvgType::Type::SvgGElement);
  REQUIRE(e != nullptr);
  e->setId("id1"s);
  doc1.addElementId(e->id(), e);
  auto e2 = doc1.getElementById("id1"s);
  CHECK(e2 != nullptr);
  CHECK(e == e2);
  auto e3 = doc1.getElementById("bad"s);
  CHECK(e3 == nullptr);
}