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
#include "palettemodel.hpp"
#include "palette.hpp"
#include "paletteitem.hpp" // Needed for magic stuff

pepp::settings::PaletteModel::PaletteModel(QObject *parent) : QAbstractListModel(parent) {}

pepp::settings::PaletteModel::PaletteModel(Palette *palette) : QAbstractListModel(nullptr), _palette(palette) {}

pepp::settings::Palette *pepp::settings::PaletteModel::palette() const { return _palette; }

void pepp::settings::PaletteModel::setPalette(Palette *palette) {
  if (palette == _palette) return;
  beginResetModel();
  _palette = palette;
  endResetModel();
}

int pepp::settings::PaletteModel::rowCount(const QModelIndex &parent) const {
  if (_palette == nullptr) return 0;
  else return static_cast<int>(PaletteRole::Total);
}

QVariant pepp::settings::PaletteModel::data(const QModelIndex &index, int role) const {
  static const int total = static_cast<int>(PaletteRole::Total);
  if (!index.isValid() || std::cmp_less(index.row(), 0) || std::cmp_greater_equal(index.row(), total))
    return QVariant();
  switch (role) {
  case Qt::DisplayRole: return PaletteRoleHelper::prettyString(static_cast<PaletteRole>(index.row()));
  case (int)Role::PaletteRoleRole: return QVariant::fromValue(static_cast<PaletteRole>(index.row()));
  case (int)Role::PaletteItemRole: return QVariant::fromValue(_palette->item(index.row()));
  case (int)Role::RequiresMonoFontRole:
    return PaletteRoleHelper::requiresMonoFont(static_cast<PaletteRole>(index.row()));
  default: return {};
  }
}

QHash<int, QByteArray> pepp::settings::PaletteModel::roleNames() const {
  static const QHash<int, QByteArray> ret{{Qt::DisplayRole, "display"},
                                          {(int)Role::PaletteRoleRole, "paletteRole"},
                                          {(int)Role::PaletteItemRole, "paletteItem"},
                                          {(int)Role::RequiresMonoFontRole, "requiresMonoFont"}};
  return ret;
}

pepp::settings::PaletteFilterModel::PaletteFilterModel(QObject *parent) {}

QVariant pepp::settings::PaletteFilterModel::category() const {
  if (_cat.has_value()) return QVariant::fromValue(_cat.value());
  else return {};
}

void pepp::settings::PaletteFilterModel::setCategory(QVariant cat) {
  bool ok = false;
  cat.toInt(&ok);
  if (ok) {
    if (_cat.has_value() && (int)_cat.value() == cat.toInt()) return;
    _cat = static_cast<PaletteCategory>(cat.toInt());
    emit invalidateRowsFilter();
    emit categoryChanged();
  } else {
    if (!_cat.has_value()) return;
    _cat.reset();
    emit invalidateRowsFilter();
    emit categoryChanged();
  }
}

bool pepp::settings::PaletteFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  if (!_cat.has_value()) return true;
  PaletteRole role = static_cast<PaletteRole>(source_row);
  PaletteCategory cat = categoryForRole(role);
  return cat == _cat.value();
}
