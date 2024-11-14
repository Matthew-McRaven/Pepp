#include "palette.hpp"
#include "paletteitem.hpp"

pepp::settings::PaletteCategoryModel::PaletteCategoryModel(QObject *parent) : QAbstractListModel(parent) {}

int pepp::settings::PaletteCategoryModel::rowCount(const QModelIndex &parent) const {
  static const auto meta = QMetaEnum::fromType<PaletteCategory>();
  return meta.keyCount();
}

bool pepp::settings::PaletteCategoryModel::removeRows(int row, int count, const QModelIndex &parent) { return false; }

QVariant pepp::settings::PaletteCategoryModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount({}) || index.column() != 0) return {};
  static const auto meta = QMetaEnum::fromType<PaletteCategory>();
  switch (role) {
  case Qt::DisplayRole: return QVariant::fromValue(QString(meta.key(index.row())));
  case Qt::UserRole + 1: return QVariant::fromValue(meta.value(index.row()));
  default: return {};
  }
}

Qt::ItemFlags pepp::settings::PaletteCategoryModel::flags(const QModelIndex &index) const {
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> pepp::settings::PaletteCategoryModel::roleNames() const {
  static const QHash<int, QByteArray> ret = {{Qt::DisplayRole, "display"}, {Qt::UserRole + 1, "value"}};
  return ret;
}

pepp::settings::PaletteItem *pepp::settings::Palette::item(int role) { return _items[role]; }

pepp::settings::PaletteItem *pepp::settings::Palette::item(int role) const { return _items[role]; }

pepp::settings::PaletteItem *pepp::settings::Palette::item(PaletteRole role) { return item((int)role); }

pepp::settings::PaletteItem *pepp::settings::Palette::item(PaletteRole role) const { return item((int)role); }

QJsonObject pepp::settings::Palette::toJson() {
  QJsonObject doc;

  //  Save font structure to document
  doc["name"] = "dummy";
  doc["version"] = _version;

  QJsonArray prefData;

  //  Save individual preferences to an array
  for (int it = 0; it < _items.size(); it++) {
    auto p = _items[it];
    auto asJSON = p->toJson();
    if (auto parent = p->parent(); !parent) { // intentionally blank. negated to convert nested to chained if.
    } else if (auto role = itemToRole(parent); role != -1)
      asJSON["parent"] = PaletteRoleHelper::string(static_cast<PaletteRole>(role));
    asJSON["name"] = PaletteRoleHelper::string(static_cast<PaletteRole>(it));
    prefData.append(asJSON);
  }
  doc["paletteItems"] = prefData;

  return doc;
}

pepp::settings::Palette::Palette(QObject *parent) : QObject(parent) {
  _items.resize(static_cast<int>(PaletteRole::Total));
  loadDefaults();
  QFile f("X:/test.json");
  f.open(QIODevice::WriteOnly);
  f.write(QJsonDocument(toJson()).toJson());
  f.close();
}

int pepp::settings::Palette::itemToRole(const PaletteItem *item) const {
  if (item == nullptr) return -1;
  for (int it = 0; it < static_cast<int>(PaletteRole::Total); it++)
    if (_items[it] == item) return it;
  return -1;
}

void pepp::settings::Palette::loadDefaults() {
  for (int it = 0; it < static_cast<int>(PaletteRole::Total); it++) {
    if (_items[it] != nullptr) continue;
    _items[it] = new PaletteItem({}, this);
  }
}
