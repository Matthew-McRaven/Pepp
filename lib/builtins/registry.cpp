/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "registry.hpp"
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QStringConverter>
#include "book.hpp"
#include "elements.hpp"
#include "figure.hpp"
#include "macro/macro.hpp"
#include "macro/parse.hpp"
using namespace Qt::StringLiterals;
builtins::Registry::Registry(void *asm_toolchains, QString directory) {
  _usingExternalFigures = (directory != builtins::default_book_path);
  for (const auto &bookPath : detail::enumerateBooks(directory)) {
    auto book = detail::loadBook(bookPath);
    if (book == nullptr) qWarning("%s", u"Failed to load book at %1"_s.arg(bookPath).toStdString().c_str());
    else if (findBook(book->name()) != nullptr) qFatal("Duplicate book");
    else _books.push_back(book);
  }
}

QList<QSharedPointer<const builtins::Book>> builtins::Registry::books() const { return _books; }

QSharedPointer<const builtins::Book> builtins::Registry::findBook(QString name) {
  using namespace Qt::StringLiterals;
  QList<QSharedPointer<const builtins::Book>> temp;
  for (auto &bookPtr : _books) {
    if (bookPtr->name() == name) {
      temp.push_back(bookPtr);
    }
  }
  if (auto length = temp.length(); length == 0) return nullptr;
  else if (length == 1) return temp.first();
  else {
    qDebug() << u"More than one copy of book {}"_s.arg(name);
    return nullptr;
  }
}

// Helper method to open a file and read all of its bytes
QByteArray read(QString path) {
  QFile asFile(path);
  asFile.open(QFile::ReadOnly);
  auto bytes = asFile.readAll();
  asFile.close();
  return bytes;
}

builtins::Element *builtins::detail::loadElement(QString elementPath) {
  auto element = new builtins::Element();
  QString data = read(elementPath);
  element->contents = data;
  element->generated = false;
  return element;
}

builtins::Element *builtins::detail::generateElement(QString fromElementPath, void *asm_toolchains) {
  // TODO: Revist when assembler toolchain works
  return nullptr;
}

builtins::Test *builtins::detail::loadTest(QString testDirPath) {
  auto test = new builtins::Test();
  QDirIterator dir(testDirPath);
  while (dir.hasNext()) {
    auto file = dir.next();
    QString data = read(file);
    data.replace("\r", "");
    if (file.endsWith("input.txt")) test->input = data;
    else if (file.endsWith("output.txt")) test->output = data;
  }
  return test;
}

QSharedPointer<builtins::Figure> builtins::detail::loadFigure(QString manifestPath) {
  auto manifestDir = QFileInfo(manifestPath).dir();
  // Read figure manifest get field names;
  auto manifestBytes = read(manifestPath);
  auto manifest = QJsonDocument::fromJson(manifestBytes);
  QString chFig = manifest["name"].toString();
  // Chapter and figure are separated by : in a manifest file.
  if (chFig.indexOf(":") == -1) {
    qWarning("Invalid figure name %s", chFig.toStdString().c_str());
    return nullptr;
  }
  auto chFigSplit = chFig.split(":");
  auto chapterName = chFigSplit[0];
  auto figureName = chFigSplit[1];

  // Extract the architecture string and convert it to the correct enum.
  auto archStr = manifest["arch"].toString();
  bool okay = false;
  auto archInt =
      QMetaEnum::fromType<builtins::Architecture>().keyToValue(archStr.toUpper().toStdString().data(), &okay);
  auto arch = static_cast<builtins::Architecture>(archInt);
  if (!okay) {
    qWarning("Invalid figure architecture: %s", archStr.toStdString().c_str());
    return nullptr;
  }

  builtins::Abstraction level = builtins::Abstraction::NONE;
  if (manifest.object().contains("abstraction")) {
    auto levelStr = manifest["abstraction"].toString();
    auto levelInt =
        QMetaEnum::fromType<builtins::Abstraction>().keyToValue(levelStr.toUpper().toStdString().data(), &okay);
    level = static_cast<builtins::Abstraction>(levelInt);
    if (!okay) {
      qWarning("Invalid abstraction: %s", levelStr.toStdString().c_str());
      return nullptr;
    }
  }

  // TODO: decide between "figure" and "problem" based on type field.
  auto figure = QSharedPointer<builtins::Figure>::create(arch, level, "Figure", chapterName, figureName);
  figure->setIsOS(manifest["is_os"].toBool(false));
  figure->setIsHidden(manifest["hidden"].toBool(false));
  if (manifest["description"].isString()) figure->setDescription(manifest["description"].toString());

  // Add tests
  auto ios = manifest["ios"];
  auto iosArray = ios.toArray();
  for (auto ioDir : std::as_const(iosArray)) {
    auto io = loadTest(manifestDir.absoluteFilePath(ioDir.toString()));
    if (io == nullptr) {
      qWarning("Invalid IO: %s", ioDir.toString().toStdString().c_str());
      return nullptr;
    }
    figure->addTest(io);
  }

  // Add elements
  auto items = manifest["items"];          // The key in the manifest which contains elements
  auto itemsArray = items.toObject();      // Get the element name:value pairs as a map
  auto itemsArrayKeys = itemsArray.keys(); // Make the name:value pairs iterable
  for (const auto &language : std::as_const(itemsArrayKeys)) {
    // Perform templatization on manifest values.
    QString itemTemplatePath = itemsArray[language].toString();
    auto itemPath = itemTemplatePath.replace("{ch}", chapterName).replace("{fig}", figureName);

    // Load the figure,
    auto item = loadElement(manifestDir.absoluteFilePath(itemPath));

    if (item == nullptr) {
      qWarning("Invalid item: %s", itemPath.toStdString().c_str());
      return nullptr;
    }
    // And then assign its parent, programming language
    item->figure = figure; // Not set in addElement, must be done manually.
    item->language = language;
    figure->addElement(language, item);
  }

  auto default_element = manifest["default_element"];
  figure->setDefaultElement(default_element.toString());

  return figure;
}

QList<QSharedPointer<macro::Parsed>> builtins::detail::loadMacro(QString manifestPath) {
  QList<QSharedPointer<macro::Parsed>> ret;
  auto manifestDir = QFileInfo(manifestPath).dir();
  // Read macro manifest get macro names;
  auto manifestBytes = read(manifestPath);
  auto manifest = QJsonDocument::fromJson(manifestBytes);

  // Add elements
  auto items = manifest["items"];          // The key in the manifest which contains elements
  auto itemsArray = items.toObject();      // Get the element name:value pairs as a map
  auto itemsArrayKeys = itemsArray.keys(); // Make the name:value pairs iterable
  for (const auto &name : std::as_const(itemsArrayKeys)) {
    // Perform templatization on manifest values.
    QString itemTemplatePath = itemsArray[name].toString();
    auto itemPath = itemTemplatePath.replace("{name}", name);

    // Load the macro
    auto macroText = read(manifestDir.absoluteFilePath(itemPath));
    auto macroBody = macroText.sliced(macroText.indexOf("\n") + 1);
    auto parsed = macro::analyze_macro_definition(macroText);
    bool isHidden = manifest["hidden"].toBool(false);
    auto family = manifest["family"].toString("");

    if (!std::get<0>(parsed)) {
      qWarning("Invalid macro: %s", itemPath.toStdString().c_str());
      return {};
    }
    auto macro = QSharedPointer<macro::Parsed>::create(std::get<1>(parsed), std::get<2>(parsed), macroBody,
                                                       manifest["arch"].toString(), family, isHidden);
    ret.push_back(macro);
  }
  return ret;
}

void builtins::detail::linkFigureOS(QString manifestPath, QSharedPointer<Figure> figure,
                                    QSharedPointer<const Book> book) {
  // Read figure manifest to determine if figure is an OS, or if it links
  // against an existing figure.
  auto manifestBytes = read(manifestPath);
  auto manifest = QJsonDocument::fromJson(manifestBytes);
  auto isOs = manifest["is_os"];
  if (isOs.isBool() && isOs.toBool()) return;
  QString chFig = manifest["default_os"].toString();
  if (chFig.isEmpty()) return;
  // Chapter and figure are separated by : in a manifest file.
  if (chFig.indexOf(":") == -1) qFatal("Invalid OS figure name");
  auto osChFigSplit = chFig.split(":");
  auto osChapterName = osChFigSplit[0];
  auto osFigureName = osChFigSplit[1];
  auto os = book->findFigure(osChapterName, osFigureName);
  if (!os) {
    qWarning("Could not find OS for %s", chFig.toStdString().c_str());
  }
  figure->setDefaultOS(os.data());
}

QSharedPointer<builtins::Book> builtins::detail::loadBook(QString tocPath) {
  static const auto bookNameKey = "bookName";
  // Read ToC to get book name
  auto tocBytes = read(tocPath);
  auto toc = QJsonDocument::fromJson(tocBytes);
  if (toc[bookNameKey].isUndefined()) return nullptr;
  // And create a book object to stick figures in
  auto book = QSharedPointer<builtins::Book>::create(toc[bookNameKey].toString());

  // Explore the book's subdirectories, looking for figures and macros.
  QDirIterator iter(QFileInfo(tocPath).dir().absolutePath(), QDirIterator::Subdirectories);
  // Maintain a list of figures that need to be linked to their default OS
  QList<std::tuple<QString, QSharedPointer<builtins::Figure>>> revisit;
  while (iter.hasNext()) {
    auto next = iter.next();

    // If the file is named as a figure manifest, parse the figure and insert
    // into book
    if (next.endsWith("figure.json")) {
      auto figure = loadFigure(next);
      if (figure == nullptr) qWarning("%s", u"Failed to load figure %1"_s.arg(next).toStdString().c_str());
      else {
        revisit.push_back({next, figure});
        book->addFigure(figure);
      }
    }
    // If the file is named as a problem manifest, parse the figure and insert
    // into book
    if (next.endsWith("problem.json")) {
      auto problem = loadFigure(next);
      if (problem == nullptr) qWarning("%s", u"Failed to load problem %1"_s.arg(next).toStdString().c_str());
      else {
        revisit.push_back({next, problem});
        book->addProblem(problem);
      }
    }

    // If the file is name as a macro manifest, parse the macro and insert into
    // book
    else if (next.endsWith("macro.json")) {
      auto macros = loadMacro(next);
      for (auto &macro : macros) {
        if (macro == nullptr) qWarning("%s", u"Failed to load macro %1"_s.arg(next).toStdString().c_str());
        else book->addMacro(macro);
      }
    }
  }

  // Revist all figures and attempt to link to default OS
  for (auto &[path, figure] : revisit) linkFigureOS(path, figure, book);

  return book;
}

QList<QString> builtins::detail::enumerateBooks(QString prefix) {
  QList<QString> ret;
  QDirIterator iter(prefix);
  // Walk the QRC (or filesystem), looking for folders than contain book
  // manifests (toc.json)
  while (iter.hasNext()) {
    auto next = iter.next();
    auto maybeManifest = QFile(QDir(next).filePath("toc.json"));
    if (maybeManifest.exists()) // Attempt to parse the directory as a book
      ret.push_back(maybeManifest.fileName());
  }
  return ret;
}
