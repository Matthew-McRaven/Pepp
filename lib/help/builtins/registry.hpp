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
#include "enums/constants.hpp"
namespace macro {
class Parsed;
}

namespace builtins {
class Test;
class Figure;
class Element;
static const char *default_book_path = ":/books";

class Registry {
public:
  struct Assembler {
    virtual ~Assembler() = default;
    virtual QVariant operator()(const QString &os, const QString &user) = 0;
  };
  struct Formatter {
    virtual ~Formatter() = default;
    virtual QString operator()(QVariant assembled) = 0;
  };
  // Crawling the Qt help system to create books is handled inside CTOR.
  explicit Registry(QString directory = default_book_path);
  QList<QSharedPointer<const builtins::Book>> books() const;
  QSharedPointer<const builtins::Book> findBook(QString name) const;
  bool usingExternalFigures() const { return _usingExternalFigures; }
  void addDependency(const Element *dependent, const Element *dependee);
  QString contentFor(Element &element);
  void addAssembler(pepp::Architecture arch, std::unique_ptr<Assembler> &&assembler);
  void addFormatter(pepp::Architecture arch, QString format, std::unique_ptr<Formatter> &&formatter);

private:
  using _Figure = QSharedPointer<builtins::Figure>;
  using _Macro = QList<QSharedPointer<macro::Parsed>>;
  std::variant<std::monostate, _Figure, _Macro> loadManifestV2(const QJsonDocument &manifest, const QString &path);
  std::variant<std::monostate, _Figure, _Macro> loadFigureV2(const QJsonDocument &manifest, const QString &path);
  std::variant<std::monostate, _Figure, _Macro> loadMacroV2(const QJsonDocument &manifest, const QString &path);

  QSharedPointer<::builtins::Book> loadBook(QString tocPath);
  bool _usingExternalFigures = false;
  QList<QSharedPointer<const builtins::Book>> _books;
  // Given an element, determine which element it depends on.
  QMap<const Element * /*dependent*/, const Element * /*dependee*/> _dependencies;
  // Given an element, determine which elements depend on it.
  QMap<const Element * /*dependee*/, QList<const Element *> /*dependents*/> _dependees;
  void computeDependencies(const Element *dependee);
  QMap<const Element *, QString> _contents;
  // Use std::map so that unique pointers are less painful. QMap COW features do not interact well.
  std::map<pepp::Architecture, std::unique_ptr<Assembler>> _assemblers;
  std::map<QPair<pepp::Architecture, QString>, std::unique_ptr<Formatter>> _formatters;
};

namespace detail {
::builtins::Test *loadTest(QString testDirPath);
void linkFigureOS(QString manifestPath, QSharedPointer<::builtins::Figure> figure,
                  QSharedPointer<const builtins::Book> book);
QList<QString> enumerateBooks(QString prefix);

} // end namespace detail
} // end namespace builtins
