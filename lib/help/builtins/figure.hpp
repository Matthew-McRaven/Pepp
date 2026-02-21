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

#pragma once
#include <QtCore>
#include "fragment.hpp"
#include "project/features.hpp"
namespace builtins {

/*!
 * \brief Represents a single figure in a textbook
 *
 * A figure is composed of multiple textual fragments, and may include a set of
 * test input:output pairs.
 *
 * This class is meant to be usable in both C++ and QML, so some Q_PROPERTYs
 * have a public API as a variant, but also provide a typesafe API for C++.
 * \see builtins::Figure#_tests \see builtins::Figure#_namedFragments
 *
 */
class Figure : public QObject {
  Q_OBJECT
  Q_PROPERTY(pepp::Architecture arch READ arch CONSTANT);
  Q_PROPERTY(pepp::Abstraction level READ level CONSTANT);
  Q_PROPERTY(pepp::Features features READ features CONSTANT);
  Q_PROPERTY(QString prefix READ prefix CONSTANT);
  Q_PROPERTY(QString chapterName READ chapterName CONSTANT);
  Q_PROPERTY(QString figureName READ figureName CONSTANT);
  Q_PROPERTY(QString description READ description CONSTANT);
  Q_PROPERTY(bool isOS READ isOS WRITE setIsOS NOTIFY isOSChanged);
  Q_PROPERTY(bool isHidden READ isHidden NOTIFY isHiddenChanged);
  Q_PROPERTY(bool isProblem READ isProblem CONSTANT);
  Q_PROPERTY(const Figure *defaultOS READ defaultOS WRITE setDefaultOS NOTIFY defaultOSChanged);
  // Must use variants if we want these to be accessed from QML.
  // We provide a type safe version, which should be used instead if in C++.
  // See builtins::Test for properties
  Q_PROPERTY(QVariantList tests READ tests NOTIFY testsChanged);
  // See builtins::Element for available properties
  Q_PROPERTY(QVariantMap fragments READ namedFragments NOTIFY fragmentsChanged);
  Q_PROPERTY(QString defaultFragmentName READ defaultFragmentName CONSTANT);

public:
  Figure(pepp::Architecture arch, pepp::Abstraction level, pepp::Features feats, QString prefix, QString chapter,
         QString figure, bool isProblem = false);

  ~Figure();

  pepp::Architecture arch() const;
  pepp::Abstraction level() const;
  pepp::Features features() const;

  QString prefix() const;
  QString chapterName() const;
  QString figureName() const;
  bool isProblem() const;

  QString description() const;
  void setDescription(QString description);

  bool isOS() const;
  bool setIsOS(bool value);

  bool isHidden() const;
  bool setIsHidden(bool value);

  const Figure *defaultOS() const;
  bool setDefaultOS(const Figure *);

  const QList<const builtins::Test *> typesafeTests() const;
  // Creates variant list on-the-fly, please limit # of calls.
  QVariantList tests() const;
  // Transfer ownership to this. Must be deleted in this object's destructor
  void addTest(const builtins::Test *test);

  const builtins::Fragment *findFragment(QString name) const;
  const QMap<QString, const builtins::Fragment *> typesafeNamedFragments() const;
  const QList<const Fragment *> &typesafeFragments() const;
  // Creates variant map on-the-fly, please limit # of calls.
  QVariantMap namedFragments() const;
  // Transfer ownership to this. Must be deleted in this object's destructor
  bool addFragment(const builtins::Fragment *fragment);
  QString defaultFragmentName() const;
  void setDefaultFragmentName(QString name);

  Q_INVOKABLE QString defaultFragmentText() const;
  Q_INVOKABLE QString defaultOSText() const;

signals:
  void isOSChanged();
  void isHiddenChanged();
  void defaultOSChanged();
  void testsChanged();
  void fragmentsChanged();

private:
  const pepp::Architecture _arch;
  const pepp::Abstraction _level;
  const pepp::Features _features = pepp::Features::None;
  const QString _prefix, _chapterName, _figureName;
  const bool _isProblem = false;
  QString _description{};
  bool _isOS = false, _isHidden = false;
  // Non-owning
  const Figure *_defaultOS = nullptr;
  // Owns pointers
  QList<const builtins::Test *> _tests = {};
  // Owns pointers
  QMap<QString, const Fragment *> _namedFragments = {};
  QList<const Fragment *> _allFragments = {};
  QString _defaultFragmentName = {};
};
} // end namespace builtins
Q_DECLARE_METATYPE(builtins::Figure);
