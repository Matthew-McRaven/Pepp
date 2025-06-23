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

#include "book.hpp"
#include "figure.hpp"

#include "toolchain/macro/declaration.hpp"
builtins::Book::Book(QString name) : QObject(nullptr), _name(name) {}

QString builtins::Book::name() const { return _name; }

const QList<QSharedPointer<builtins::Figure>> builtins::Book::figures() const {
  return _figures;
}

QSharedPointer<const builtins::Figure> builtins::Book::findFigure(QString chapter, QString figure) const {
  using namespace Qt::StringLiterals;
  QList<QSharedPointer<const builtins::Figure>> temp;
  for (auto figurePtr : _figures) {
    if (figurePtr->chapterName() == chapter &&
        figurePtr->figureName() == figure) {
      temp.push_back(figurePtr);
    }
  }
  if (auto length = temp.length(); length == 0)
    return nullptr;
  else if (length == 1)
    return temp.first();
  else {
    qDebug() << u"More than one copy of figure {}.{}"_s.arg(chapter).arg(figure);
    return nullptr;
  }
}

bool builtins::Book::addFigure(QSharedPointer<Figure> figure) {
  // TODO: Adding N figures will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for large N.
  if (findFigure(figure->chapterName(), figure->figureName()) == nullptr) {
    _figures.push_back(figure);
    return true;
  } else
    return false;
}

const QList<QSharedPointer<builtins::Figure>> builtins::Book::problems() const {
  return _problems;
}

QSharedPointer<const builtins::Figure>
builtins::Book::findProblem(QString chapter, QString problem) const {
  using namespace Qt::StringLiterals;
  QList<QSharedPointer<const builtins::Figure>> temp;
  for (auto figurePtr : _problems) {
    if (figurePtr->chapterName() == chapter &&
        figurePtr->figureName() == problem) {
      temp.push_back(figurePtr);
    }
  }
  if (auto length = temp.length(); length == 0)
    return nullptr;
  else if (length == 1)
    return temp.first();
  else {
    qDebug() << u"More than one copy of problem {}.{}"_s.arg(chapter).arg(problem);
    return nullptr;
  }
}

bool builtins::Book::addProblem(QSharedPointer<Figure> problem) {
  // TODO: Adding N figures will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for large N.
  if (findProblem(problem->chapterName(), problem->figureName()) == nullptr) {
    _problems.push_back(problem);
    return true;
  } else
    return false;
}

const QList<QSharedPointer<macro::Declaration>> builtins::Book::macros() const {
  return _macros;
}

QSharedPointer<const macro::Declaration>
builtins::Book::findMacro(QString name) const {
  using namespace Qt::StringLiterals;
  QList<QSharedPointer<const macro::Declaration>> temp;
  for (const auto &macroPtr : _macros) {
    if (macroPtr->name() == name)
      temp.push_back(macroPtr);
  }
  if (auto length = temp.length(); length == 0)
    return nullptr;
  else if (length == 1)
    return temp.first();
  else {
    qDebug() << u"More than one copy of macro {}"_s.arg(name);
    return nullptr;
  }
}

bool builtins::Book::addMacro(QSharedPointer<macro::Declaration> macro) {
  // TODO: Adding N macros will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for large N.
  if (findMacro(macro->name()) == nullptr) {
    _macros.push_back(macro);
    return true;
  } else
    return false;
}
