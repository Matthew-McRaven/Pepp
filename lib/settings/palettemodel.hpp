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
#include <QObject>
#include <QtQmlIntegration>
#include "./constants.hpp"

namespace pepp::settings {
class Palette;
class PaletteModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_STRUCTURED_VALUE
  Q_PROPERTY(Palette *palette READ palette WRITE setPalette NOTIFY paletteChanged)

public:
  enum class Role : int {
    // Role which contails PaletteRole for a given item... sorry for the name.
    PaletteRoleRole = Qt::UserRole + 1,
    PaletteItemRole,
    RequiresMonoFontRole,
  };
  explicit PaletteModel(QObject *parent = nullptr);
  explicit Q_INVOKABLE PaletteModel(Palette *palette);
  Palette *palette() const;
  void setPalette(Palette *palette);
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
signals:
  void paletteChanged();

private:
  Palette *_palette{nullptr};
};

class PaletteFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariant category READ category WRITE setCategory NOTIFY categoryChanged)
public:
  explicit PaletteFilterModel(QObject *parent = nullptr);
  QVariant category() const;
  void setCategory(QVariant category);
signals:
  void categoryChanged();

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
  std::optional<PaletteCategory> _cat = std::nullopt;
};
} // namespace pepp::settings
