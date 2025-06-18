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

#pragma once

#include <QObject>

// Needed to prevent type_traits from complaining that Book has throwing dtor.
#include "book.hpp"
namespace macro {
class Parsed;
}

namespace builtins {
class Test;
class Figure;
class Element;
class Element2;
static const char *default_book_path = ":/books";
class Registry {
public:
  // Crawling the Qt help system to create books is handled inside CTOR.
  explicit Registry(QString directory = default_book_path);
  QList<QSharedPointer<const builtins::Book>> books() const;
  QSharedPointer<const builtins::Book> findBook(QString name);
  bool usingExternalFigures() const { return _usingExternalFigures; }
  void addDependency(const Element2 *dependent, const Element2 *dependee);
  QString contentFor(Element2 &element);

private:
  using _Figure = QSharedPointer<builtins::Figure>;
  using _Macro = QSharedPointer<macro::Parsed>;
  std::variant<std::monostate, _Figure, _Macro> loadManifestV2(const QJsonDocument &manifest, const QString &path);
  QSharedPointer<::builtins::Book> loadBook(QString tocPath);
  bool _usingExternalFigures = false;
  QList<QSharedPointer<const builtins::Book>> _books;
  // Given an element, determine which element it depends on.
  QMap<const Element2 * /*dependent*/, const Element2 * /*dependee*/> _dependencies;
  // Given an element, determine which elements depend on it.
  QMap<const Element2 * /*dependee*/, QList<const Element2 *> /*dependents*/> _dependees;
  void computeDependencies(const Element2 *dependee);
  QMap<const Element2 *, QString> _contents;
};

namespace detail {
::builtins::Element *loadElement(QString elementPath);
::builtins::Element *generateElement(QString fromElementPath, void *asm_toolchains);
::builtins::Test *loadTest(QString testDirPath);
QSharedPointer<builtins::Figure> loadFigure(QString manifestPath);
QSharedPointer<builtins::Figure> loadProblem(QString manifestPath);
void linkFigureOS(QString manifestPath, QSharedPointer<::builtins::Figure> figure,
                  QSharedPointer<const builtins::Book> book);
QList<QSharedPointer<::macro::Parsed>> loadMacro(QString manifestPath);

QList<QString> enumerateBooks(QString prefix);

} // end namespace detail
} // end namespace builtins
