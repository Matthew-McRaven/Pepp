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
#include "elements.hpp"
#include <QtCore>
#include "builtins_globals.hpp"
namespace builtins {

/*!
 * \brief Represents a single figure in a textbook
 *
 * A figure is composed of multiple textual elements, and may include a set of
 * test input:output pairs.
 *
 * This class is meant to be usable in both C++ and QML, so some Q_PROPERTYs
 * have a public API as a variant, but also provide a typesafe API for C++.
 * \see builtins::Figure#_tests \see builtins::Figure#_elements
 *
 */
class BUILTINS_EXPORT Figure : public QObject {
  Q_OBJECT
  Q_PROPERTY(builtins::Architecture arch READ arch CONSTANT);
  Q_PROPERTY(QString chapterName READ chapterName CONSTANT);
  Q_PROPERTY(QString figureName READ figureName CONSTANT);
  Q_PROPERTY(bool isOS READ isOS WRITE setIsOS NOTIFY isOSChanged);
  Q_PROPERTY(const Figure *defaultOS READ defaultOS WRITE setDefaultOS NOTIFY
                 defaultOSChanged);
  // Must use variants if we want these to be accessed from QML.
  // We provide a type safe version, which should be used instead if in C++.
  // See builtins::Test for properties
  Q_PROPERTY(QVariantList tests READ tests NOTIFY testsChanged);
  // See builtins::Element for available properties
  Q_PROPERTY(QVariantMap elements READ elements NOTIFY elementsChanged);

public:
  Figure(Architecture arch, QString chapter, QString figure);
  ~Figure();

  builtins::Architecture arch() const;

  QString chapterName() const;

  QString figureName() const;

  bool isOS() const;
  bool setIsOS(bool value);

  const Figure *defaultOS() const;
  bool setDefaultOS(const Figure *);

  const QList<const builtins::Test *> typesafeTests() const;
  // Creates variant list on-the-fly, please limit # of calls.
  QVariantList tests() const;
  // Transfer ownership to this. Must be deleted in this object's destructor
  void addTest(const builtins::Test *test);

  const QMap<QString, const builtins::Element *> typesafeElements() const;
  // Creates variant map on-the-fly, please limit # of calls.
  QVariantMap elements() const;
  // Transfer ownership to this. Must be deleted in this object's destructor
  bool addElement(QString name, const builtins::Element *element);

signals:
  void isOSChanged();
  void defaultOSChanged();
  void testsChanged();
  void elementsChanged();

private:
  const Architecture _arch;
  const QString _chapterName, _figureName;
  bool _isOS = false;
  // Non-owning
  const Figure *_defaultOS = nullptr;
  // Owns pointers
  QList<const builtins::Test *> _tests = {};
  // Owns pointers
  QMap<QString, const builtins::Element *> _elements = {};
};
} // end namespace builtins
Q_DECLARE_METATYPE(builtins::Figure);
