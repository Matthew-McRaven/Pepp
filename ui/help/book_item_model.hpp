/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include <QHash>
#include <QStandardItemModel>
#include <QString>
#include "QtQmlIntegration/qqmlintegration.h"

namespace builtins {

#define SHARED_CONSTANT(type, name, value)                                                                             \
  static inline const type name = value;                                                                               \
  Q_PROPERTY(type name MEMBER name CONSTANT)

/*!
 * \brief Contains constants for item model roles to be shared between QML and
 * C++
 *
 * These roles are used to
 */
class FigureConstants : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  //! A role which contains a string description of the scope of the current
  //! item. For example, "book", or "figure". It is used to interpret the
  //! payload field
  SHARED_CONSTANT(quint32, FIG_ROLE_KIND, Qt::UserRole + 1);
  /*!
   * \brief A role which contains figures or help documentation depending on the
   * value of \sa FIG_ROLE_KIND.
   *
   * If FIG_ROLE_KIND == "figure", then this field contains a \sa
   * builtins::Figure If FIG_ROLE_KIND == "book", then this field contains a
   * QVariantList of \sa builtins::Figure.
   */
  SHARED_CONSTANT(quint32, FIG_ROLE_PAYLOAD, Qt::UserRole + 2);

  SHARED_CONSTANT(quint32, FIG_ROLE_EDITION, Qt::UserRole + 3);
  SHARED_CONSTANT(quint32, FIG_ROLE_ABSTRACTION, Qt::UserRole + 4);
  SHARED_CONSTANT(quint32, FIG_ROLE_ARCHITECTURE, Qt::UserRole + 5);
};

class Registry;
class BookModel : public QStandardItemModel {
  Q_OBJECT
public:
  BookModel(QObject *parent = nullptr);
  QHash<int, QByteArray> roleNames() const override;

private:
  QSharedPointer<builtins::Registry> _registry;
};
} // namespace builtins
Q_DECLARE_METATYPE(builtins::BookModel);
