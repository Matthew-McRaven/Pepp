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
  else if (role != Qt::DisplayRole) return QVariant();
  switch (role) {
  case Qt::DisplayRole: return PaletteRoleHelper::string(static_cast<PaletteRole>(index.row()));
  case (int)Role::PaletteItemRole: return QVariant::fromValue(_palette->item(index.row()));
  default: return {};
  }
}

QHash<int, QByteArray> pepp::settings::PaletteModel::roleNames() const {
  static const QHash<int, QByteArray> ret{{Qt::DisplayRole, "display"}, {(int)Role::PaletteItemRole, "paletteItem"}};
  return ret;
}

pepp::settings::PaletterFilterModel::PaletterFilterModel(QObject *parent) {}

QVariant pepp::settings::PaletterFilterModel::category() const {
  if (_cat.has_value()) return QVariant::fromValue(_cat.value());
  else return {};
}

void pepp::settings::PaletterFilterModel::setCategory(QVariant cat) {
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

bool pepp::settings::PaletterFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  if (!_cat.has_value()) return true;
  PaletteRole role = static_cast<PaletteRole>(source_row);
  PaletteCategory cat = categoryForRole(role);
  return cat == _cat.value();
}
