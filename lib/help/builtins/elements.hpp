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

#include <QSharedPointer>
#include <QString>
#include <QtCore>
#include <optional>
#include "enums/constants.hpp"
namespace builtins {

class Figure;
/*!
 * \brief Contains a unit of content that makes up a help item
 */
struct Element : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(bool generated MEMBER generated);
  Q_PROPERTY(QString language MEMBER language);
  Q_PROPERTY(QString content READ contents);
  Q_PROPERTY(QWeakPointer<Figure> figure MEMBER figure);

public:
  //! Is the element created dynamicaly at runtime (e.g., pepo/pepb/peph/pepl),
  //! or is it "baked in" to the QRC (pep/c)
  bool generated;
  //! The programming language this element is written in
  QString language;
  //! The textual contents of the element
  QString contents() const;
  std::function<QString()> contentsFn = Element::empty;
  //! The figure which contains this element. Needed to access default OS / test
  //! items.
  QWeakPointer<Figure> figure;

protected:
  static inline QString empty() { return ""; }
  mutable std::optional<QString> _contents;
};

struct Element2 : public Element {
private:
  Q_OBJECT
  Q_PROPERTY(QString name MEMBER name);
  Q_PROPERTY(bool isDefault MEMBER isDefault)
  Q_PROPERTY(bool isHidden MEMBER isHidden)
  Q_PROPERTY(QString copyType MEMBER copyType);

public:
  QString name, copyType;
  bool isDefault = false, isHidden = false;
  QString exportPath = "";
};

/*!
 * \brief A single input:output pair that can be used to unit test an
 * figure.
 */
struct Test : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(QVariant input MEMBER input);
  Q_PROPERTY(QVariant output MEMBER output);

public:
  //! If present, it is a string containing the input on which the figure should
  //! be run.
  QVariant input;
  //! If present, it is the required output of the figure when run on the
  //! supplied input.
  QVariant output;
};

struct Macro : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(pepp::Architecture arch MEMBER arch);
  Q_PROPERTY(QString name MEMBER name);
  Q_PROPERTY(QString text MEMBER text);

public:
  pepp::Architecture arch;
  QString name;
  QString text;
};
} // end namespace builtins
