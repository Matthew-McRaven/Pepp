/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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
#include <spdlog/spdlog.h>
#include "book.hpp"
#include "figure.hpp"
#include "fragment.hpp"
#include "toolchain/macro/declaration.hpp"
#include "toolchain2/macro/parse.hpp"


using namespace Qt::StringLiterals;

builtins::Registry::Registry(std::unique_ptr<FilesystemProvider> fs) : _fs(std::move(fs)) {
  for (const auto &bookPath : _fs->enumerateFiles("")) {
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

void builtins::Registry::addDependency(const Fragment *dependent, const Fragment *dependee) {
  _dependencies[dependent] = dependee;
  if (!_dependees.contains(dependee)) _dependees[dependee] = QList<const Fragment *>();
  _dependees[dependee].append(dependent);
}

QString builtins::Registry::contentFor(Fragment &element) {
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

void builtins::Registry::addFormatter(pepp::Architecture arch, QString format,
                                      std::unique_ptr<Formatter> &&formatter) {
  auto p = QPair<pepp::Architecture, QString>(arch, format);
  _formatters[p] = std::move(formatter);
}

builtins::Test *builtins::Registry::loadTest(QString testDirPath) {
  auto test = new builtins::Test();
  QDirIterator dir(testDirPath);
  while (dir.hasNext()) {
    auto file = dir.next();
    if (!QFileInfo(file).isFile() && !file.endsWith(".txt")) continue;
    QString data = _fs->readFile(file);
    data.replace("\r", "");
    if (file.endsWith("input.txt")) test->input = data;
    else if (file.endsWith("output.txt")) test->output = data;
  }
  return test;
}

void builtins::Registry::linkFigureOS(const QString &manifestPath, QSharedPointer<Figure> figure,
                                      QSharedPointer<const Book> book) {
  // Read figure manifest to determine if figure is an OS, or if it links
  // against an existing figure.
  auto manifestBytes = _fs->readFile(manifestPath);
  auto manifest = QJsonDocument::fromJson(manifestBytes.toUtf8());
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

void builtins::Registry::computeDependencies(const Fragment *dependee) {
  // Compute dependee's value using correct assembler/compiler toolchain.
  if (!_dependees.contains(dependee)) return;

  auto dependents = _dependees.value(dependee);
  if (dependents.isEmpty()) return;
  auto figure = dependee->figure.lock();
  if (figure == nullptr) {
    SPDLOG_WARN("Dependee %s has no figure", dependee->name.toStdString());
    return;
  }
  auto assembler = _assemblers.find(figure->arch());
  if (assembler == _assemblers.end()) {
    const auto arch = pepp::arch_as_string(dependee->figure.lock()->arch());
    SPDLOG_WARN("No assembler for architecture {}", arch);
    return;
  }
  auto os = figure->defaultOS();
  auto os_text = os->findFragment(os->defaultFragmentName())->contents();
  auto assembled = (*assembler).second->operator()(os_text, dependee->contents());
  // For each dependent of the thing being assembled, compute the desired output.
  // This avoids needing to assemble the same dependee multiple times.
  for (const auto &dependent : std::as_const(dependents)) {
    auto figure = dependent->figure.lock();
    if (figure == nullptr) {
      SPDLOG_WARN("Dependent {} has no figure", dependent->name.toStdString());
      _contents[dependent] = "";
      continue;
    }
    auto formatter = _formatters.find({figure->arch(), dependent->language});
    if (formatter == _formatters.end()) {
      const auto arch = pepp::arch_as_string(dependee->figure.lock()->arch());
      SPDLOG_WARN("No formatter for architecture {} and format {}", arch, dependent->language.toStdString());

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
  for (int pos = -1; (pos = input.indexOf('\n', pos + 1)) != -1;) lineStarts.append(pos + 1);
  lineStarts.append(input.size() + 1);
  auto startIdx = absolute_index(lineStarts, startLine, 0),
       endIdx = absolute_index(lineStarts, endLine, input.size() - 1);
  if (endIdx < startIdx) std::swap(startIdx, endIdx);
  return input.mid(lineStarts[startIdx], lineStarts[endIdx] - lineStarts[startIdx]);
}

std::string templatize(std::string input, std::map<std::string, std::string> substitutions) {
  for (const auto &[key, value] : substitutions) {
    const std::string token = "{" + key + "}";
    for (std::size_t pos = input.find(token); pos != std::string::npos; pos = input.find(token, pos + value.size()))
      input.replace(pos, token.size(), value);
  }
  return input;
}

void templateize(nlohmann::json &object, std::map<std::string, std::string> substitutions) {
  for (auto &[key, value] : object.items()) {
    if (value.is_object()) {
      templateize(value, substitutions);
    } else if (value.is_string()) {
      object[key] = templatize(value.get<std::string>(), substitutions);
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
  bool okay = false;
  auto ret = pepp::string_to_arch(key.toStdString(), &okay);
  if (!okay) {
    SPDLOG_WARN("Invalid figure architecture: {}", key.toStdString());
    return std::nullopt;
  }
  return ret;
}

std::optional<pepp::Abstraction> abs_from_str(const QString &key) {
  auto keyStr = key.toUpper().toStdString();
  bool okay = false;
  auto ret = pepp::string_to_level(key.toStdString(), &okay);
  if (!okay) {
    SPDLOG_WARN("Invalid figure abstraction: {}", key.toStdString());
    return std::nullopt;
  }
  return ret;
}

std::string str_of(const nlohmann::json &manifest, const std::string &key) {
  if (!manifest.contains(key)) return {};
  else if (!manifest[key].is_string()) {
    qWarning("Manifest key %s is not a str", key.c_str());
    return {};
  }
  return manifest[key].get<std::string>();
}
bool bool_of(const nlohmann::json &manifest, const std::string &key, bool _default = false) {
  if (!manifest.contains(key)) return _default;
  else if (!manifest[key].is_boolean()) {
    qWarning("Manifest key %s is not a boolean", key.c_str());
    return _default;
  }
  return manifest[key].get<bool>();
}

int int_of(const nlohmann::json &manifest, const std::string &key, int _default = 0) {
  if (!manifest.contains(key)) return _default;
  else if (!manifest[key].is_number_integer()) {
    qWarning("Manifest key %s is not an integer", key.c_str());
    return _default;
  }
  return manifest[key].get<int>();
}
int intAt(const nlohmann::json &lines, std::size_t i, int fallback) {
  return (i < lines.size() && lines[i].is_number_integer()) ? lines[i].get<int>() : fallback;
};

} // namespace

builtins::Fragment *builtins::Registry::loadFragment(const nlohmann::json &item, const QDir &manifestDir,
                                                     Figure *parent) {
  // Use smart pointer to avoid cleanup on error paths.
  auto fragment = std::make_unique<builtins::Fragment>();
  fragment->name = QString::fromStdString(str_of(item, "name"));
  if (!item.contains("format") || !item["format"].is_string()) {
    qWarning("Invalid fragment format for %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else fragment->language = QString::fromStdString(str_of(item, "format"));

  fragment->isHidden = bool_of(item, "hidden", false);
  if (auto isDefault = bool_of(item, "isDefault", false); isDefault && fragment->isHidden) {
    qWarning("isDefault is incompatible with isHidden for %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else if (isDefault && fragment->name.isEmpty()) {
    qWarning("Default fragment must be named %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else fragment->isDefault = isDefault;

  fragment->copyType = QString::fromStdString(str_of(item, "copyType"));
  fragment->exportPath = QString::fromStdString(str_of(item, "export"));

  if (!item.contains("from") || !item["from"].is_object()) {
    qWarning("from element must be an object %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else if (auto from = item["from"]; true) {
    // Require one of "element" or "file" to be present.
    if (from.contains("file") && from["file"].is_string()) {
      auto relPath = QString::fromStdString(str_of(from, "file"));
      auto absPath = manifestDir.absoluteFilePath(relPath);
      fragment->contentsFn = [this, absPath]() { return _fs->readFile(absPath); };
    } else if (from.contains("element") && from["element"].is_string()) {
      auto key = QString::fromStdString(str_of(from, "element"));
      auto dependee = parent->findFragment(key);
      if (dependee == nullptr) {
        qWarning("Fragment %s not found in figure %s", key.toStdString().c_str(),
                 parent->figureName().toStdString().c_str());
        return nullptr;
      }

      addDependency(fragment.get(), dependee);
      auto elementPtr = fragment.get();
      fragment->contentsFn = [this, elementPtr]() { return contentFor(*elementPtr); };
    } else {
      qWarning("Did not specify a valid source");
      return nullptr;
    }

    // Extract a subset of lines from the file.
    if (from.contains("lines") && from["lines"].is_array()) {
      auto contentsFn = fragment->contentsFn;
      if (auto lines = from["lines"]; !lines[0].is_number() || !lines[1].is_number()) {
        qWarning("Invalid line numbers for %s", fragment->name.toStdString().c_str());
        return nullptr;
      } else {
        int startLine = intAt(lines, 0, 0), endLine = intAt(lines, 1, -1);
        fragment->contentsFn = [=]() {
          auto contents = contentsFn();
          return selectLines(contents, startLine, endLine);
        };
      }
    }
  }

  return fragment.release();
}

std::variant<std::monostate, builtins::Registry::_Figure, builtins::Registry::_Macro>
builtins::Registry::loadManifestV2(const nlohmann::json &manifest, const QString &path) {
  const auto type = str_of(manifest, "type");
  if (type == "figure" || type == "problem") return loadFigureV2(manifest, path);
  else if (type == "macro") return loadMacroV2(manifest, path);
  else return std::monostate();
}

std::variant<std::monostate, builtins::Registry::_Figure, builtins::Registry::_Macro>
builtins::Registry::loadFigureV2(const nlohmann::json &manifest, const QString &path) {
  const auto manifestDir = QFileInfo(path).dir();
  const auto type = str_of(manifest, "type");
  // Extract chapter/figure name
  auto chFig = ch_fig_from_str(QString::fromStdString(str_of(manifest, "name")));
  if (!chFig) return std::monostate();
  auto [chapterName, figureName] = *chFig;

  // Extract architecture / abstraction from manifest into enumerated constants
  auto arch = pepp::Architecture::NO_ARCH;
  auto level = pepp::Abstraction::NO_ABS;
  if (auto maybeArch = arch_from_str(QString::fromStdString(str_of(manifest, "arch"))); !maybeArch)
    return std::monostate();
  else arch = *maybeArch;
  if (auto maybeLevel = abs_from_str(QString::fromStdString(str_of(manifest, "abstraction"))); !maybeLevel)
    return std::monostate();
  else level = *maybeLevel;
  auto feats = pepp::parse_features(manifest.value("features", std::string{}));

  const auto prefix = type == "problem" ? "Problem" : "Figure";
  auto figure =
      QSharedPointer<builtins::Figure>::create(arch, level, feats, prefix, chapterName, figureName, type == "problem");
  figure->setIsOS(bool_of(manifest, "isOS", false));
  figure->setIsHidden(bool_of(manifest, "hidden", false));
  if (manifest.contains("description") && manifest["description"].is_string())
    figure->setDescription(QString::fromStdString(str_of(manifest, "description")));

  // Add tests

  if (manifest.contains("tests") && manifest["tests"].is_array()) {
    for (const auto &tests_it : std::as_const(manifest["tests"])) {
      if (!tests_it.is_string()) {
        qWarning("Invalid test directory in manifest");
        return std::monostate();
      }
      auto io_str = tests_it.get<std::string>();
      auto abs_dir = manifestDir.absoluteFilePath(QString::fromStdString(io_str));
      auto io = loadTest(abs_dir);
      if (io == nullptr) {
        qWarning("Invalid IO: %s", io_str.c_str());
        return std::monostate();
      }
      figure->addTest(io);
    }
  }

  auto substitutions =
      std::map<std::string, std::string>{{"ch", chapterName.toStdString()}, {"fig", figureName.toStdString()}};

  // Add elements
  std::optional<QString> _default = std::nullopt;
  if (!manifest.contains("fragments") || !manifest["fragments"].is_array()) {
    qWarning("Manifest missing required fragments array");
    return std::monostate();
  }
  for (const auto &frag_it : std::as_const(manifest["fragments"])) {
    nlohmann::json templatized = frag_it;
    templateize(templatized, substitutions);
    auto item = loadFragment(templatized, manifestDir, &*figure);
    if (item == nullptr) {
      qWarning("Failed to load element %s", str_of(templatized, "name").c_str());
      continue;
    }
    item->figure = figure;
    figure->addFragment(item);
    if (item->isDefault && !_default.has_value()) _default = item->name;
  }

  figure->setDefaultFragmentName(_default.value_or("pepo"));

  return figure;
}

std::variant<std::monostate, builtins::Registry::_Figure, builtins::Registry::_Macro>
builtins::Registry::loadMacroV2(const nlohmann::json &manifest, const QString &path) {
  QList<QSharedPointer<macro::Declaration>> ret;
  auto manifestDir = QFileInfo(path).dir();

  if (!manifest.contains("fragments") || !manifest["fragments"].is_object()) {
    qWarning("Manifest missing required fragments object");
    return std::monostate();
  }
  // Add elements

  for (const auto &[key, value] : manifest["fragments"].items()) {
    if (!value.is_string()) {
      SPDLOG_WARN("Fragment name must be a string");
      continue;
    }

    // Perform templatization on manifest values.

    const auto substitutions = std::map<std::string, std::string>{{"name", key}};
    const auto path = templatize(value.get<std::string>(), substitutions);

    // Load the macro
    const auto macroText = _fs->readFile(manifestDir.absoluteFilePath(QString::fromStdString(path)));
    const auto macroBody = macroText.sliced(macroText.indexOf("\n") + 1);
    auto parsed = macro::analyze_macro_definition(macroText);
    const bool isHidden = bool_of(manifest, "isHidden", false);
    const auto family = str_of(manifest, "family");
    const auto arch = str_of(manifest, "arch");

    if (!std::get<0>(parsed)) {
      qWarning("Invalid macro: %s", path.c_str());
      return {};
    }

    auto macro = QSharedPointer<macro::Declaration>::create(std::get<1>(parsed), std::get<2>(parsed), macroBody,
                                                            QString::fromStdString(arch),
                                                            QString::fromStdString(family), isHidden);
    ret.push_back(macro);
  }
  return ret;
}

QSharedPointer<builtins::Book> builtins::Registry::loadBook(QString tocPath) {
  static const auto bookNameKey = "bookName";
  // Read ToC to get book name
  auto tocBytes = _fs->readFile(tocPath).toStdString();
  QSharedPointer<builtins::Book> book = nullptr;
  try {
    auto toc = nlohmann::json::parse(tocBytes);
    if (!toc.contains(bookNameKey)) return nullptr;
    // And create a book object to stick figures in
    book = QSharedPointer<builtins::Book>::create(QString::fromStdString(toc[bookNameKey].get<std::string>()));
  } catch (const std::exception &e) {
    qWarning("%s", u"Failed to parse toc.json for %1: %2"_s.arg(tocPath).arg(e.what()).toStdString().c_str());
    return nullptr;
  }

  // Explore the book's subdirectories, looking for figures and macros.
  QDirIterator iter(QFileInfo(tocPath).dir().absolutePath(), QDirIterator::Subdirectories);
  // Maintain a list of figures that need to be linked to their default OS
  QList<std::tuple<QString, QSharedPointer<builtins::Figure>>> revisit;
  while (iter.hasNext()) {
    auto next = iter.next();

    if (next.endsWith("manifest.json")) {
      auto manifestBytes = _fs->readFile(next).toStdString();
      nlohmann::json manifest;
      try {
        manifest = nlohmann::json::parse(manifestBytes);
      } catch (const std::exception &e) {
        qWarning("%s", u"Failed to parse manifest.json for %1: %2"_s.arg(next).arg(e.what()).toStdString().c_str());
        continue;
      }

      if (auto v = int_of(manifest, "version", 0); v == 2) {
        auto item = loadManifestV2(manifest, next);
        if (std::holds_alternative<std::monostate>(item)) {
          qWarning("%s", u"Failed to load manifest v2 %1"_s.arg(next).toStdString().c_str());
        } else if (std::holds_alternative<_Figure>(item)) {
          auto figure = std::get<_Figure>(item);
          revisit.push_back({next, figure});
          if (figure->isProblem()) book->addProblem(figure);
          else book->addFigure(figure);
        } else if (std::holds_alternative<_Macro>(item)) {
          auto macros = std::get<_Macro>(item);
          for (auto &macro : macros) {
            if (macro == nullptr) qWarning("%s", u"Failed to load macro %1"_s.arg(next).toStdString().c_str());
            else book->addMacro(macro);
          }
        }
      } else {
        qWarning("%s", u"Unknown manifest version %1 for %2"_s.arg(v).arg(next).toStdString().c_str());
      }
    }
    // If the file is name as a macro manifest, parse the macro and insert into book
    else if (next.endsWith("macro.json")) {
      qFatal("Should not be called");
    }
  }

  // Revist all figures and attempt to link to default OS
  for (auto &[path, figure] : revisit) linkFigureOS(path, figure, book);

  return book;
}

QString builtins::QRCFSProvider::readFile(const QString &path) {
  QFile asFile(resolve(path));
  (void)asFile.open(QFile::ReadOnly);
  auto bytes = asFile.readAll();
  asFile.close();
  return bytes;
}

QStringList builtins::QRCFSProvider::enumerateFiles(const QString &directory) {
  QList<QString> ret;
  QDirIterator iter(resolve(directory));
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

bool builtins::QRCFSProvider::using_external_figures() const { return _prefix != default_book_path; }

QString builtins::QRCFSProvider::resolve(QString path) const {
  if (QDir::isAbsolutePath(path)) return path;
  return _prefix + "/" + path;
}

std::unique_ptr<builtins::Registry::FilesystemProvider> builtins::makeQRCFSProvider(QString prefix) {
  return std::make_unique<QRCFSProvider>(std::move(prefix));
}
