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
#include <QVector>
#include <QtQmlIntegration>
#include "sim3/api/traced/trace_iterator.hpp"

class Flag {
public:
  explicit Flag(QString name, std::function<bool()> value);
  QString name() const;
  bool value() const;

private:
  QString _name;
  std::function<bool()> _fn;
};

//  Read only class for change in status bits
class FlagModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

public:
  enum class Roles { Value = Qt::UserRole + 1 };
  Q_ENUM(Roles);

  explicit FlagModel(QObject *parent = nullptr);
  ~FlagModel() = default;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void appendFlag(QSharedPointer<Flag> flag);

public slots:
  void onUpdateGUI();

protected: //  Role Names must be under protected
  QHash<int, QByteArray> roleNames() const override;
  QVector<QSharedPointer<Flag>> _flags;
};
