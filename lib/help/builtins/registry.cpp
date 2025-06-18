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
#include "toolchain/macro/macro.hpp"
#include "toolchain/macro/parse.hpp"

// Helper method to open a file and read all of its bytes
QByteArray read(QString path) {
  QFile asFile(path);
  asFile.open(QFile::ReadOnly);
  auto bytes = asFile.readAll();
  asFile.close();
  return bytes;
}

using namespace Qt::StringLiterals;
builtins::Registry::Registry(QString directory) {
  _usingExternalFigures = (directory != builtins::default_book_path);
  for (const auto &bookPath : detail::enumerateBooks(directory)) {
    auto book = loadBook(bookPath);
    if (book == nullptr) qWarning("%s", u"Failed to load book at %1"_s.arg(bookPath).toStdString().c_str());
    else if (findBook(book->name()) != nullptr) qFatal("Duplicate book");
    else _books.push_back(book);
  }
}

QList<QSharedPointer<const builtins::Book>> builtins::Registry::books() const { return _books; }

QSharedPointer<const builtins::Book> builtins::Registry::findBook(QString name) const {
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

void builtins::Registry::addDependency(const Element2 *dependent, const Element2 *dependee) {
  _dependencies[dependent] = dependee;
  if (!_dependees.contains(dependee)) _dependees[dependee] = QList<const Element2 *>();
  _dependees[dependee].append(dependent);
}

QString builtins::Registry::contentFor(Element2 &element) {
  // If the element has already been computed, use that value.
  if (_contents.contains(&element)) return _contents[&element];

  // Otherwise find the element that we depend on, and assemble it.
  auto dependee = _dependencies.value(&element, nullptr);
  if (dependee == nullptr) return _contents[&element] = "";
  else computeDependencies(dependee);
  return _contents[&element];
}

void builtins::Registry::addAssembler(pepp::Architecture arch, std::unique_ptr<Assembler> &&assembler) {
  _assemblers[arch] = std::move(assembler);
}

void builtins::Registry::addFormatter(pepp::Architecture arch, QString format, std::unique_ptr<Formatter> &&formatter) {
  auto p = QPair<pepp::Architecture, QString>(arch, format);
  _formatters[p] = std::move(formatter);
}

void builtins::Registry::computeDependencies(const Element2 *dependee) {
  // Compute dependee's value using correct assembler/compiler toolchain.
  if (!_dependees.contains(dependee)) return;

  auto dependents = _dependees.value(dependee);
  if (dependents.isEmpty()) return;
  auto figure = dependee->figure.lock();
  if (figure == nullptr) {
    qWarning("Dependee %s has no figure", dependee->name.toStdString().c_str());
    return;
  }
  auto assembler = _assemblers.find(figure->arch());
  if (assembler == _assemblers.end()) {
    qWarning("No assembler for architecture %s",
             QMetaEnum::fromType<pepp::Architecture>().valueToKey(static_cast<int>(dependee->figure.lock()->arch())));
    return;
  }
  auto os = figure->defaultOS();
  auto os_text = os->findElement(os->defaultElement())->contents();
  auto assembled = (*assembler).second->operator()(os_text, dependee->contents());
  // For each dependent of the thing being assembled, compute the desired output.
  // This avoids needing to assemble the same dependee multiple times.
  for (const auto &dependent : std::as_const(dependents)) {
    auto figure = dependent->figure.lock();
    if (figure == nullptr) {
      qWarning("Dependent %s has no figure", dependent->name.toStdString().c_str());
      _contents[dependent] = "";
      continue;
    }
    auto formatter = _formatters.find({figure->arch(), dependent->language});
    if (formatter == _formatters.end()) {
      qWarning("No formatter for architecture %s and format %s",
               QMetaEnum::fromType<pepp::Architecture>().valueToKey(static_cast<int>(figure->arch())),
               dependent->language.toStdString().c_str());
      _contents[dependent] = "";
      continue;
    }
    if (!_contents.contains(dependent)) _contents[dependent] = (*formatter).second->operator()(assembled);
  }
}

namespace {

int absolute_index(QList<int> &list, int index, int _default = 0) {
  if (index >= 0 && index < list.length()) return index;
  else if (index < 0 && index >= -list.length()) return list.length() + index;
  else return _default;
}

QString selectLines(QString &input, int startLine, int endLine) {
  QList<int> lineStarts = {0};
  for (int line = 0, pos = -1; (pos = input.indexOf('\n', pos + 1)) != -1; ++line) lineStarts.append(pos + 1);
  auto startIdx = absolute_index(lineStarts, startLine, 0),
       endIdx = absolute_index(lineStarts, endLine, input.size() - 1);
  if (endIdx < startIdx) std::swap(startIdx, endIdx);
  return input.mid(startIdx, endIdx);
}

QString loadFromFile(QString path) { return read(path); }

void templateize(QJsonObject &object, QMap<QString, QString> substitutions) {
  auto keys = object.keys();
  for (const auto &key : std::as_const(keys)) {
    auto value = object[key];
    if (value.isObject()) {
      auto innerObject = value.toObject();
      templateize(innerObject, substitutions);
      object[key] = innerObject;
    } else if (value.isString()) {
      auto newValue = value.toString();
      for (const auto &[key, value] : substitutions.asKeyValueRange()) newValue.replace(u"{%1}"_s.arg(key), value);
      object[key] = newValue;
    }
  }
}

std::optional<std::pair<QString, QString>> ch_fig_from_str(const QString &key) {
  // Chapter and figure are separated by : in a manifest file.
  if (key.indexOf(":") == -1) {
    qWarning("Invalid chapter:figure name %s", key.toStdString().c_str());
    return std::nullopt;
  }
  auto chFigSplit = key.split(":");
  if (chFigSplit.length() != 2) {
    qWarning("Invalid chapter:figure name %s", key.toStdString().c_str());
    return std::nullopt;
  }
  return std::make_pair(chFigSplit[0], chFigSplit[1]);
}

std::optional<pepp::Architecture> arch_from_str(const QString &key) {
  auto keyStr = key.toUpper().toStdString();
  bool okay = false;
  auto archInt = QMetaEnum::fromType<pepp::Architecture>().keyToValue(keyStr.data(), &okay);
  if (!okay) {
    qWarning("Invalid figure architecture: %s", keyStr.data());
    return std::nullopt;
  }
  return static_cast<pepp::Architecture>(archInt);
}

std::optional<pepp::Abstraction> abs_from_str(const QString &key) {
  auto keyStr = key.toUpper().toStdString();
  bool okay = false;
  auto archInt = QMetaEnum::fromType<pepp::Abstraction>().keyToValue(keyStr.data(), &okay);
  if (!okay) {
    qWarning("Invalid figure abstraction: %s", keyStr.data());
    return std::nullopt;
  }
  return static_cast<pepp::Abstraction>(archInt);
}

::builtins::Element2 *loadElement2(const QJsonObject &item, const QDir &manifestDir, builtins::Figure *parent,
                                   builtins::Registry *registry) {
  // Use smart pointer to avoid cleanup on error paths.
  auto element = std::make_unique<builtins::Element2>();
  element->name = item["name"].toString("");
  if (!item["format"].isString()) {
    qWarning("Invalid element format for %s", element->name.toStdString().c_str());
    return nullptr;
  } else element->language = item["format"].toString();

  element->isHidden = item["hidden"].toBool(false);
  if (auto isDefault = item["isDefault"].toBool(false); isDefault && element->isHidden) {
    qWarning("isDefault is incompatible with isHidden for %s", element->name.toStdString().c_str());
    return nullptr;
  } else if (isDefault && element->name.isEmpty()) {
    qWarning("Default element must be named %s", element->name.toStdString().c_str());
    return nullptr;
  } else element->isDefault = isDefault;

  element->copyType = item["copyType"].toString("");
  element->exportPath = item["export"].toString("");

  if (!item["from"].isObject()) {
    qWarning("from element must be an object %s", element->name.toStdString().c_str());
    return nullptr;
  } else if (auto from = item["from"].toObject(); true) {
    // Require one of "element" or "file" to be present.
    if (from["file"].isString()) {
      auto absPath = manifestDir.absoluteFilePath(from["file"].toString());
      element->contentsFn = [=]() { return loadFromFile(absPath); };
    } else if (from["element"].isString()) {
      auto dependee = parent->findElement(from["element"].toString());
      if (dependee == nullptr) {
        qWarning("Element %s not found in figure %s", from["element"].toString().toStdString().c_str(),
                 parent->figureName().toStdString().c_str());
        return nullptr;
      }
      auto casted = dynamic_cast<const builtins::Element2 *>(dependee);
      if (casted == nullptr) {
        qWarning("Element %s is not of type Element2", from["element"].toString().toStdString().c_str());
        return nullptr;
      }
      registry->addDependency(element.get(), casted);
      auto elementPtr = element.get();
      element->contentsFn = [registry, elementPtr]() { return registry->contentFor(*elementPtr); };
    } else {
      qWarning("Did not specify a valid source");
      return nullptr;
    }

    // Extract a subset of lines from the file.
    if (from["lines"].isArray()) {
      auto contentsFn = element->contentsFn;
      if (auto lines = from["lines"].toArray(); !lines[0].isDouble() || !lines[1].isDouble()) {
        qWarning("Invalid line numbers for %s", element->name.toStdString().c_str());
        return nullptr;
      } else {
        auto startLine = lines[0].toInt(0), endLine = lines[1].toInt(-1);
        element->contentsFn = [=]() {
          auto contents = contentsFn();
          return selectLines(contents, startLine, endLine);
        };
      }
    }
  }

  return element.release();
}
} // namespace

std::variant<std::monostate, builtins::Registry::_Figure, builtins::Registry::_Macro>
builtins::Registry::loadManifestV2(const QJsonDocument &manifest, const QString &path) {
  const auto manifestDir = QFileInfo(path).dir();
  const auto type = manifest["type"].toString("").toLower();
  if (type == "figure" || type == "problem") {
    // Extract chapter/figure name
    auto chFig = ch_fig_from_str(manifest["name"].toString());
    if (!chFig) return std::monostate();
    auto [chapterName, figureName] = *chFig;

    // Extract architecture / abstraction from manifest into enumerated constants
    pepp::Architecture arch = pepp::Architecture::NO_ARCH;
    pepp::Abstraction level = pepp::Abstraction::NO_ABS;
    if (auto maybeArch = arch_from_str(manifest["arch"].toString("")); !maybeArch) return std::monostate();
    else arch = *maybeArch;
    if (auto maybeLevel = abs_from_str(manifest["abstraction"].toString("")); !maybeLevel) return std::monostate();
    else level = *maybeLevel;

    // TODO: decide between "figure" and "problem" based on type field.
    auto figure =
        QSharedPointer<builtins::Figure>::create(arch, level, "Figure", chapterName, figureName, type == "problem");
    figure->setIsOS(manifest["isOS"].toBool(false));
    figure->setIsHidden(manifest["hidden"].toBool(false));
    if (manifest["description"].isString()) figure->setDescription(manifest["description"].toString());

    // Add tests
    auto ios = manifest["tests"];
    auto iosArray = ios.toArray();
    for (auto ioDir : std::as_const(iosArray)) {
      auto io = detail::loadTest(manifestDir.absoluteFilePath(ioDir.toString()));
      if (io == nullptr) {
        qWarning("Invalid IO: %s", ioDir.toString().toStdString().c_str());
        return std::monostate();
      }
      figure->addTest(io);
    }

    auto substitutions = QMap<QString, QString>{{"ch", chapterName}, {"fig", figureName}};

    // Add elements
    auto itemsArray = manifest["items"].toArray();
    std::optional<QString> _default = std::nullopt;
    for (const auto &itemIter : std::as_const(itemsArray)) {
      // Perform templatization on manifest values.
      auto itemObject = itemIter.toObject();
      templateize(itemObject, substitutions);
      auto item = loadElement2(itemObject, manifestDir, &*figure, this);
      if (item == nullptr) {
        qWarning("Failed to load element %s", itemObject["name"].toString("").toStdString().c_str());
        continue;
      }
      item->figure = figure;
      figure->addElement(item->name, item);
      if (item->isDefault && !_default.has_value()) _default = item->language;
    }

    figure->setDefaultElement(_default.value_or("pepo"));

    return figure;
  }
  return std::monostate();
}

QSharedPointer<builtins::Book> builtins::Registry::loadBook(QString tocPath) {
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

    if (next.endsWith("manifest.json")) {
      auto manifestBytes = read(next);
      auto manifest = QJsonDocument::fromJson(manifestBytes);
      if (manifest["version"].toInt(0) == 2) {
        auto item = loadManifestV2(manifest, next);
        if (std::holds_alternative<std::monostate>(item)) {
          qWarning("%s", u"Failed to load manifest v2 %1"_s.arg(next).toStdString().c_str());
        } else if (std::holds_alternative<_Figure>(item)) {
          auto figure = std::get<_Figure>(item);
          revisit.push_back({next, figure});
          // TODO: delineate between figures and problems
          book->addFigure(figure);
        } else if (std::holds_alternative<_Macro>(item)) {
          qFatal("Failed to load macros using manifest v2");
        }
      } else {
        qWarning(
            "%s",
            u"Unknown manifest version %1 for %2"_s.arg(manifest["version"].toInt(0)).arg(next).toStdString().c_str());
      }
    }
    // If the file is named as a figure manifest, parse the figure and insert into book
    else if (next.endsWith("figure.json")) {
      auto figure = detail::loadFigure(next);
      if (figure == nullptr) qWarning("%s", u"Failed to load figure %1"_s.arg(next).toStdString().c_str());
      else {
        revisit.push_back({next, figure});
        book->addFigure(figure);
      }
    }
    // If the file is named as a problem manifest, parse the figure and insert into book
    if (next.endsWith("problem.json")) {
      auto problem = detail::loadFigure(next);
      if (problem == nullptr) qWarning("%s", u"Failed to load problem %1"_s.arg(next).toStdString().c_str());
      else {
        revisit.push_back({next, problem});
        book->addProblem(problem);
      }
    }

    // If the file is name as a macro manifest, parse the macro and insert into book
    else if (next.endsWith("macro.json")) {
      auto macros = detail::loadMacro(next);
      for (auto &macro : macros) {
        if (macro == nullptr) qWarning("%s", u"Failed to load macro %1"_s.arg(next).toStdString().c_str());
        else book->addMacro(macro);
      }
    }
  }

  // Revist all figures and attempt to link to default OS
  for (auto &[path, figure] : revisit) detail::linkFigureOS(path, figure, book);

  return book;
}

builtins::Element *builtins::detail::loadElement(QString elementPath) {
  qFatal("loadElement should no longer be reachable, but called with %s", elementPath.toStdString().c_str());
}

builtins::Element *builtins::detail::generateElement(QString fromElementPath, void *asm_toolchains) {
  qFatal("generateElement should no longer be reachable, but called with %s", fromElementPath.toStdString().c_str());
}

builtins::Test *builtins::detail::loadTest(QString testDirPath) {
  auto test = new builtins::Test();
  QDirIterator dir(testDirPath);
  while (dir.hasNext()) {
    auto file = dir.next();
    if (!QFileInfo(file).isFile() && !file.endsWith(".txt")) continue;
    QString data = read(file);
    data.replace("\r", "");
    if (file.endsWith("input.txt")) test->input = data;
    else if (file.endsWith("output.txt")) test->output = data;
  }
  return test;
}

QSharedPointer<builtins::Figure> builtins::detail::loadFigure(QString manifestPath) {
  qFatal("loadFigure should no longer be reachable, but called with %s", manifestPath.toStdString().c_str());
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
