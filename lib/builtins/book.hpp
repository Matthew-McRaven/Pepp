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

#include "macro/macro.hpp"
#include <QObject>
#include <QtCore>

namespace builtins {
class Figure;

/*!
 * \brief The Book class
 *
 * Multiple figures with the same chapter:figure name will cause a runtime
 * crash. Multiple macros with the same name and arity will cause a runtime
 * crash.
 */
class Book : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT);
  Q_PROPERTY(const QList<QSharedPointer<builtins::Figure>> figures READ figures
                 NOTIFY figuresChanged);
  Q_PROPERTY(const QList<QSharedPointer<macro::Parsed>> macros READ macros NOTIFY macrosChanged);

public:
    explicit Book(QString name);
    ~Book() noexcept = default;
    //! Return the full name of the textbook
    QString name() const;
    //! Returns the list of all figures contained by the book.
    const QList<QSharedPointer<builtins::Figure>> figures() const;
    //! If the book contains a matching figure, return that figure, otherwise
    //! return nullptr. We do not allow multiple figures with the same name.
    QSharedPointer<const builtins::Figure> findFigure(QString chapter, QString figure) const;
    //! Register an figure as part of the current book.
    //! Returns false if a figure by this name already exists, and true otherwise.
    //! If returning false, the figure was not added to the book.
    bool addFigure(QSharedPointer<builtins::Figure> figure);

    //! Returns the list of all figures contained by the book.
    const QList<QSharedPointer<builtins::Figure>> problems() const;
    //! If the book contains a matching figure, return that figure, otherwise
    //! return nullptr. We do not allow multiple figures with the same name.
    QSharedPointer<const builtins::Figure> findProblem(QString chapter, QString problem) const;
    //! Register an figure as part of the current book.
    //! Returns false if a figure by this name already exists, and true otherwise.
    //! If returning false, the figure was not added to the book.
    bool addProblem(QSharedPointer<builtins::Figure> problem);

    //! Return the list of all macros which are contained by this book.
    const QList<QSharedPointer<macro::Parsed>> macros() const;
    //! If the book contains a matching macro, return that macro, otherwise
    //! return nullptr. We do not allow multiple macros with the same name and
    //! arity.
    QSharedPointer<const macro::Parsed> findMacro(QString name) const;
    //! Register a macro as part of the current book.
    //! Returns false if a macro by this name and arity already exists, and true
    //! otherwise. If returning false, the macro was not added to the book.
    bool addMacro(QSharedPointer<macro::Parsed> macro);
signals:
  //! Emitted whenever an element or test is added to a figure, or a new figure
  //! is added.
  void figuresChanged();
  //! Emitted whenever a new macro is added.
  void macrosChanged();

private:
  QString _name;
  QList<QSharedPointer<builtins::Figure>> _figures = {}, _problems = {};
  QList<QSharedPointer<macro::Parsed>> _macros = {};
};
} // end namespace builtins
