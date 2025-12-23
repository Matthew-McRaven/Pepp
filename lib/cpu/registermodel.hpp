/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>
#include "sim3/api/traced/trace_iterator.hpp"

struct RegisterFormatter;
//  Read only class for change in Register values
class RegisterModel : public QAbstractTableModel {
  Q_OBJECT
  Q_PROPERTY(Roles Box MEMBER _box CONSTANT);
  Q_PROPERTY(Roles RightJustify MEMBER _justify CONSTANT);
  Q_PROPERTY(Roles Choices MEMBER _choices CONSTANT);
  Q_PROPERTY(Roles Selected MEMBER _selected CONSTANT);
  QML_ELEMENT

public:
  enum class Roles { Box = Qt::UserRole + 1, RightJustify, Choices, Selected };
  Q_ENUM(Roles)

  explicit RegisterModel(QObject *parent = nullptr);
  ~RegisterModel() = default;

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  // Append rows / columns to data model.
  void appendFormatters(QVector<QSharedPointer<RegisterFormatter>> formatters);
  Q_INVOKABLE qsizetype columnCharWidth(int column) const;
public slots:
  void onUpdateGUI();

private:
  uint32_t _cols = 0;
  QVector<QVector<QSharedPointer<RegisterFormatter>>> _data;
  const Roles _box = Roles::Box;
  const Roles _justify = Roles::RightJustify;
  const Roles _choices = Roles::Choices;
  const Roles _selected = Roles::Selected;

protected: //  Role Names must be under protected
  QHash<int, QByteArray> roleNames() const override;
};
