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
  case Role::SeqCircuitRole: return "Sequential Circuit";
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

pepp::settings::CategoryHelper::CategoryHelper(QObject *parent) : QObject(parent) {}

QString pepp::settings::CategoryHelper::string(PaletteCategory cat) const {
  QMetaEnum metaEnum = QMetaEnum::fromType<PaletteCategory>();
  return metaEnum.valueToKey(static_cast<int>(cat));
}

enum Ranges : uint32_t {
  GeneralCategoryStart = (uint32_t)pepp::settings::PaletteRole::BaseRole,
  GeneralCategoryEnd = (uint32_t)pepp::settings::PaletteRole::MnemonicRole,
  EditorCategoryStart = GeneralCategoryEnd,
  EditorCategoryEnd = (uint32_t)pepp::settings::PaletteRole::SeqCircuitRole,
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
