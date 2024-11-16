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

bool pepp::settings::Palette::updateFromJson(const QJsonObject &json) {
  // this gon' be shitty
  bool okay;
  // if (const QJsonValue v = json["name"]; v.isString()) _name = v.toString();
  if (auto asInt = json["version"].toInt(0); asInt != _version) {
    qDebug() << "Version mismatch in theme file. Expected " << _version << " got " << asInt;
    return false;
  }
  static const auto roles = QMetaEnum::fromType<PaletteRole>();
  // Do any global updates, like changing the value of a font for each preference to match global font.
  _name = json["name"].toString();

  //

  if (const QJsonValue v = json["paletteItems"]; v.isArray()) {
    const QJsonArray prefsObj = v.toArray();
    //  Loop through preferences
    for (const QJsonValue &prefObj : prefsObj) {
      auto asObj = prefObj.toObject();
      if (asObj.isEmpty()) continue;

      auto index = roles.keyToValue(asObj["name"].toString().toStdString().c_str(), &okay);
      if (!okay || index >= (int)PaletteRole::Total) {
        qWarning() << "Skipping preference: " << asObj["name"].toString();
        continue;
      };

      PaletteItem *parent = nullptr;
      if (!asObj.contains("parent")) {                                                     // No parent, can't assign
      } else if (auto parentString = asObj["parent"].toString(); parentString.isEmpty()) { // invalid parent
      } else if (auto parentIndex = roles.keyToValue(parentString.toStdString().c_str(), &okay);
                 !okay || index >= (int)PaletteRole::Total) { // invalid parent
      } else if (parentIndex >= index) qWarning() << "Invalid parent " << parentIndex << " for " << index;
      else parent = _items[parentIndex];
      // Block signals to prevent a cascade of partial updates.
      QSignalBlocker block(_items[index]);
      _items[index]->updateFromJson(asObj, parent);
    }
    // Disconnect signals so that we do not emit itemChanged many times.
    for (auto &p : _items) QObject::disconnect(p, &PaletteItem::preferenceChanged, this, &Palette::itemChanged);
    for (auto &p : _items) {
      if (p->parent() == nullptr) p->emitChanged();
      QObject::connect(p, &PaletteItem::preferenceChanged, this, &Palette::itemChanged);
    }
    emit itemChanged();
  }

  return true;
}

QJsonObject pepp::settings::Palette::toJson() {
  QJsonObject doc;

  //  Save font structure to document
  doc["name"] = _name;
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

QString pepp::settings::Palette::jsonString() { return QJsonDocument(toJson()).toJson(); }

pepp::settings::Palette::Palette(QObject *parent) : QObject(parent) {
  _items.resize(static_cast<int>(PaletteRole::Total), nullptr);
  loadLightDefaults();
  for (auto &p : _items) QObject::connect(p, &PaletteItem::preferenceChanged, this, &Palette::itemChanged);
}

int pepp::settings::Palette::itemToRole(const PaletteItem *item) const {
  if (item == nullptr) return -1;
  for (int it = 0; it < static_cast<int>(PaletteRole::Total); it++)
    if (_items[it] == item) return it;
  return -1;
}

void pepp::settings::Palette::loadLightDefaults() {
  static const auto defaultMono = QFont("Courier Prime", 12);
  using PO = PaletteItem::PreferenceOptions;
  using R = PaletteRoleHelper::Role;
  for (int it = 0; it < static_cast<int>(PaletteRole::Total); it++) {
    if (_items[it] != nullptr) continue;
    PaletteItem *pref = nullptr;
    switch (static_cast<PaletteRole>(it)) {
    case PaletteRoleHelper::Role::BaseRole:
      pref = new PaletteItem(PO{.fg = qRgb(0x0, 0x0, 0x0), .bg = qRgb(0xff, 0xff, 0xff), .font = QFont()}, this);
      break;
    case PaletteRoleHelper::Role::BaseMonoRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::BaseRole], .font = defaultMono}, this);
      break;
    case PaletteRoleHelper::Role::WindowRole:
      pref = new PaletteItem(
          PO{.parent = _items[(int)R::BaseRole], .fg = qRgb(0x0f, 0x0f, 0x0f), .bg = qRgb(0xee, 0xee, 0xee)}, this);
      break;
    case PaletteRoleHelper::Role::ButtonRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::BaseRole], .bg = qRgb(0xf0, 0xf0, 0xf0)}, this);
      break;
    case PaletteRoleHelper::Role::HighlightRole:
      pref = new PaletteItem(
          PO{.parent = _items[(int)R::BaseRole], .fg = qRgb(0xff, 0xff, 0xff), .bg = qRgb(0x0, 0x78, 0xd7)}, this);
      break;
    case PaletteRoleHelper::Role::TooltipRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::BaseRole], .bg = qRgb(0xff, 0xff, 0xdc)}, this);
      break;
    case PaletteRoleHelper::Role::AlternateBaseRole:
      pref = new PaletteItem(
          PO{.parent = _items[(int)R::BaseRole], .fg = qRgb(0x7f, 0x7f, 0x7f), .bg = qRgb(0x0a, 0x0a, 0x0a)}, this);
      break;
    case PaletteRoleHelper::Role::AccentRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::HighlightRole]}, this);
      break;
    case PaletteRoleHelper::Role::LightRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::BaseRole], .fg = qRgb(0x0, 0x0, 0xff)}, this);
      break;
    case PaletteRoleHelper::Role::MidLightRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::BaseRole], .bg = qRgb(0xe3, 0xe3, 0xe3)}, this);
      break;
    case PaletteRoleHelper::Role::MidRole: [[fallthrough]];
    case PaletteRoleHelper::Role::DarkRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::AlternateBaseRole], .fg = qRgb(0xf0, 0xf0, 0xf0)}, this);
      break;
    case PaletteRoleHelper::Role::ShadowRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::HighlightRole], .bg = qRgb(0x69, 0x69, 0x69)}, this);
      break;
    case PaletteRoleHelper::Role::LinkRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::BaseRole], .fg = qRgb(0x0, 0x78, 0xd7)}, this);
      break;
    case PaletteRoleHelper::Role::LinkVisitedRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::LinkRole], .fg = qRgb(0x78, 0x40, 0xa0)}, this);
      break;
    case PaletteRoleHelper::Role::BrightTextRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::HighlightRole], .bg = qRgb(0xa0, 0xa0, 0xa0)}, this);
      break;
    case PaletteRoleHelper::Role::PlaceHolderTextRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::HighlightRole], .fg = qRgb(0x7f, 0x7f, 0x7f)}, this);
      break;
      // Welcome to editor land
    case PaletteRoleHelper::Role::MnemonicRole:
      pref = new PaletteItem(
          PO{.parent = _items[(int)R::BaseMonoRole], .fg = qRgb(0x25, 0x40, 0xbd), .bg = qRgba(0xff, 0xff, 0xff, 0xff)},
          this);
      break;
    case PaletteRoleHelper::Role::SymbolRole:
      pref = new PaletteItem(
          {.parent = _items[(int)R::MnemonicRole], .fg = qRgb(0xb6, 0x7b, 0xbc), .bg = qRgba(0xff, 0xff, 0xff, 0xff)},
          this);
      pref->overrideBold(true);
      break;
    case PaletteRoleHelper::Role::DirectiveRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::MnemonicRole]}, this);
      break;
    case PaletteRoleHelper::Role::MacroRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::MnemonicRole]}, this);
      pref->overrideBold(false);
      pref->overrideItalic(true);
      break;
    case PaletteRoleHelper::Role::CharacterRole:
      pref = new PaletteItem({.parent = _items[(int)R::MnemonicRole],
                              .fg = QColor("orangered").rgba(),
                              .bg = qRgba(0xff, 0xff, 0xff, 0xff)},
                             this);
      pref->overrideBold(false);
      break;
    case PaletteRoleHelper::Role::StringRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::CharacterRole]}, this);
      break;
    case PaletteRoleHelper::Role::CommentRole:
      pref = new PaletteItem(
          {.parent = _items[(int)R::MnemonicRole], .fg = qRgb(0x66, 0xa3, 0x33), .bg = qRgba(0xff, 0xff, 0xff, 0xff)},
          this);
      pref->overrideBold(false);
      break;
    case PaletteRoleHelper::Role::RowNumberRole:
      pref = new PaletteItem(PO{.parent = _items[(int)R::BaseRole], .fg = qRgb(0x66, 0x66, 0x66)}, this);
      break;
    case PaletteRoleHelper::Role::BreakpointRole:
      pref = new PaletteItem({.parent = _items[(int)R::BaseRole], .bg = qRgb(0xff, 0xaa, 0x00)}, this);
      break;
    case PaletteRoleHelper::Role::ErrorRole:
      pref = new PaletteItem(
          PO{.parent = _items[(int)R::CommentRole], .fg = qRgb(0x00, 0x00, 0x00), .bg = qRgba(0xff, 0x00, 0x00, 0xff)},
          this);
      break;
    case PaletteRoleHelper::Role::WarningRole:
      pref = new PaletteItem(
          PO{.parent = _items[(int)R::CommentRole], .fg = qRgb(0x00, 0x00, 0x00), .bg = qRgba(0xff, 0xFF, 0xE0, 0xff)},
          this);
      break;
    case PaletteRoleHelper::Role::SeqCircuitRole:
      pref = new PaletteItem(
          PO{
              .parent = _items[(int)R::MnemonicRole],
              .fg = qRgb(0xff, 0xff, 0x00),
              .bg = qRgb(0x04, 0xab, 0x0a),
          },
          this);
      pref->overrideBold(false);
      break;
    case PaletteRoleHelper::Role::CircuitGreenRole:
      pref = new PaletteItem(
          PO{
              .parent = _items[(int)R::MnemonicRole],
              .fg = qRgb(0x0, 0x0, 0xff),
              .bg = qRgb(0xff, 0xe1, 0xff),
          },
          this);
      pref->overrideBold(false);
      break;
    default: throw std::logic_error("Should be unreachable");
    }
    _items[it] = pref;
  }
}
