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
#include "constants.hpp"

pepp::settings::PaletteRoleHelper::PaletteRoleHelper(QObject *parent) : QObject(parent) {}

QString pepp::settings::PaletteRoleHelper::string(Role role) {
  QMetaEnum metaEnum = QMetaEnum::fromType<Role>();
  return metaEnum.valueToKey(static_cast<int>(role));
}

QString pepp::settings::PaletteRoleHelper::prettyString(Role role) {
  switch (role) {
  case Role::BaseMonoRole: return "Base + Monospace Font";
  case Role::PlaceHolderTextRole: return "Placeholder Text";
  case Role::CombinationalRole: return "Combinational Logic";
  case Role::SequentialRole: return "Sequential Logic";
  case Role::CircuitPrimaryRole: return "Circuit Primary";
  case Role::CircuitSecondaryRole: return "Circuit Secondary";
  case Role::CircuitTertiaryRole: return "Circuit Tertiary";
  case Role::CircuitQuaternaryRole: return "Circuit Quaternary";
  default: {
    auto str = string(role);
    str.replace("Role", "");
    // Replace capital letters in the middle of a word with a capital+space
    static const QRegularExpression re("\\B([A-Z])");
    str.replace(re, " \\1");
    return str;
  }
  }
}

bool pepp::settings::PaletteRoleHelper::requiresMonoFont(Role role) {
  switch (role) {
  case Role::BaseMonoRole: [[fallthrough]];
  case Role::MnemonicRole: [[fallthrough]];
  case Role::SymbolRole: [[fallthrough]];
  case Role::DirectiveRole: [[fallthrough]];
  case Role::MacroRole: [[fallthrough]];
  case Role::CharacterRole: [[fallthrough]];
  case Role::StringRole: [[fallthrough]];
  case Role::CommentRole: [[fallthrough]];
  case Role::ErrorRole: [[fallthrough]];
  case Role::WarningRole: [[fallthrough]];
  case Role::CombinationalRole: [[fallthrough]];
  case Role::SequentialRole: [[fallthrough]];
  case Role::CircuitPrimaryRole: [[fallthrough]];
  case Role::CircuitSecondaryRole: [[fallthrough]];
  case Role::CircuitTertiaryRole: [[fallthrough]];
  case Role::CircuitQuaternaryRole: return true;
  default: return false;
  }
}

pepp::settings::CategoryHelper::CategoryHelper(QObject *parent) : QObject(parent) {}

QString pepp::settings::CategoryHelper::string(PaletteCategory cat) const {
  QMetaEnum metaEnum = QMetaEnum::fromType<PaletteCategory>();
  return metaEnum.valueToKey(static_cast<int>(cat));
}

enum Ranges : uint32_t {
  GeneralCategoryStart = (uint32_t)pepp::settings::PaletteRole::BaseRole,
  GeneralCategoryEnd = (uint32_t)pepp::settings::PaletteRole::MnemonicRole,
  EditorCategoryStart = GeneralCategoryEnd,
  EditorCategoryEnd = (uint32_t)pepp::settings::PaletteRole::CombinationalRole,
  CircuitCategoryStart = EditorCategoryEnd,
  CircuitCategoryEnd = (uint32_t)pepp::settings::PaletteRole::Total,
};
pepp::settings::PaletteCategory pepp::settings::categoryForRole(PaletteRole role) {
  uint32_t asInt = static_cast<uint32_t>(role);
  if (asInt < GeneralCategoryEnd) return PaletteCategory::General;
  else if (asInt < EditorCategoryEnd) return PaletteCategory::Editor;
  else return PaletteCategory::Circuit;
}

pepp::settings::ValidPaletteParentModel::ValidPaletteParentModel(QObject *parent) : QAbstractListModel(parent) {}

pepp::settings::ValidPaletteParentModel::ValidPaletteParentModel(PaletteRole role) : QAbstractListModel(nullptr) {}

pepp::settings::PaletteRole pepp::settings::ValidPaletteParentModel::role() const { return _role; }

void pepp::settings::ValidPaletteParentModel::setRole(PaletteRole role) {
  if (_role == role) return;
  beginResetModel();
  _role = role;
  endResetModel();
  emit roleChanged();
}

int pepp::settings::ValidPaletteParentModel::rowCount(const QModelIndex &parent) const {
  return static_cast<int>(_role);
}

QVariant pepp::settings::ValidPaletteParentModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(_role)) return QVariant();
  switch (role) {
  case Qt::DisplayRole: return pepp::settings::PaletteRoleHelper::prettyString(static_cast<PaletteRole>(index.row()));
  case Qt::UserRole + 1: return QVariant::fromValue(static_cast<PaletteRole>(index.row()));
  default: return QVariant();
  }
}

QHash<int, QByteArray> pepp::settings::ValidPaletteParentModel::roleNames() const {
  return {{Qt::DisplayRole, "display"}, {Qt::UserRole + 1, "id"}};
}
