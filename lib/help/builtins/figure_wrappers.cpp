/*
 * Copyright (c) 2023-2027 J. Stanley Warford, Matthew McRaven
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

#include "figure_wrappers.hpp"
#include "spdlog/spdlog.h"

#include <QQmlEngine>

QVariantList builtins::FigureWrapper::tests() const {
  QVariantList v;
  auto tests = _wrapper->tests();
  for (const auto &test : tests) {
    auto qt_test = new TestWrapper(std::make_shared<pepp::Test>(*test));
    qt_test->setParent(const_cast<FigureWrapper *>(this));
    v.append(QVariant::fromValue(qt_test));
  }
  return v;
}

QVariantMap builtins::FigureWrapper::namedFragments() const {
  // Convert type-correct map to a QVariantMap, which can be accessed natively in QML
  QVariantMap v;
  auto frags = _wrapper->named_fragments();
  for (const auto &[key, value] : frags) {
    auto qt_frag = new FragmentWrapper(std::make_shared<pepp::Fragment>(*value));
    qt_frag->setParent(const_cast<FigureWrapper *>(this));
    v[QString::fromStdString(key)] = QVariant::fromValue(qt_frag);
  }
  return v;
}

std::strong_ordering builtins::FigureWrapper::operator<=>(const FigureWrapper &rhs) const {
  auto lhs_book = _wrapper->book().lock(), rhs_book = rhs._wrapper->book().lock();
  if (!lhs_book || !rhs_book) throw std::logic_error("You have no book. This should be impossible");
  auto lhs_ed = pepp::edition_number(lhs_book->name()), rhs_ed = pepp::edition_number(rhs_book->name());
  if (lhs_ed != rhs_ed) return lhs_ed <=> rhs_ed;
  if (auto cmp = _wrapper->name_chapter() <=> rhs._wrapper->name_chapter(); cmp != 0) return cmp;
  return _wrapper->name_figure() <=> rhs._wrapper->name_figure();
}

const QVariantList builtins::BookWrapper::figures() const {
  // Avoid reallocation on every call by caching return value.
  if (!_figures.isEmpty()) return _figures;

  for (const auto &figure : _wrapper->figures()) {
    auto qt_figure = new FigureWrapper(std::make_shared<pepp::Figure>(*figure));
    // Avoid double free by marking the new object as owned by C++.
    QQmlEngine::setObjectOwnership(qt_figure, QQmlEngine::CppOwnership);
    qt_figure->setParent(const_cast<BookWrapper *>(this));
    _figures.emplaceBack(QVariant::fromValue(qt_figure));
  }
  return _figures;
}

const QVariantList builtins::BookWrapper::macros() const {
  // Avoid reallocation on every call by caching return value.
  if (!_macros.isEmpty()) return _macros;

  for (const auto &macro : _wrapper->macros()) {
    auto qt_macro = new MacroWrapper(std::make_shared<pepp::MacroFile>(*macro));
    // Avoid double free by marking the new object as owned by C++.
    QQmlEngine::setObjectOwnership(qt_macro, QQmlEngine::CppOwnership);
    qt_macro->setParent(const_cast<BookWrapper *>(this));
    _macros.append(QVariant::fromValue(qt_macro));
  }
  return _macros;
}

builtins::QtFilesystemProvider::QtFilesystemProvider(QString base_path) : _base(base_path) {}

std::string builtins::QtFilesystemProvider::join(const std::string &path, const std::string &parent) const {
  return QDir(QString::fromStdString(parent)).filePath(QString::fromStdString(path)).toStdString();
}

std::string builtins::QtFilesystemProvider::dir_of(const std::string &path) const {
  auto resolved = resolve(_base, QString::fromStdString(path));
  QFileInfo info(resolved);
  // If file, return the directory of the file. If a directory, return it unchanged.
  if (info.isFile()) return QDir(_base).relativeFilePath(info.path()).toStdString();
  else return QDir(_base).relativeFilePath(resolved).toStdString();
}

std::string builtins::QtFilesystemProvider::read_file(const std::string &path) const {
  auto resolved = resolve(_base, QString::fromStdString(path));
  QFile file(resolved);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    SPDLOG_WARN("Failed to open file {}", resolved.toStdString());
    return "";
  }
  const auto contents = file.readAll();
  return contents.toStdString();
}

std::vector<std::string> builtins::QtFilesystemProvider::enumerate_files(const std::string &directory) const {
  std::vector<std::string> ret;
  auto resolved = resolve(_base, QString::fromStdString(directory));
  if (!QFileInfo(resolved).isDir()) return ret;
  QDirIterator it(resolved, QDir::NoDotAndDotDot | QDir::AllEntries);
  while (it.hasNext()) {
    auto next = it.next();
    ret.push_back(QDir(_base).relativeFilePath(next).toStdString());
  }
  return ret;
}

bool builtins::QtFilesystemProvider::using_external_figures() const { return default_book_path != _base; }

QString builtins::QtFilesystemProvider::resolve(const QString &base, const QString &path) {
  // If already absolute, use as-is; otherwise resolve against _base.
  if (QDir::isAbsolutePath(path)) return QDir::cleanPath(path);
  return QDir::cleanPath(QDir(base).filePath(path));
}
