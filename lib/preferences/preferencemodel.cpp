#include "preferencemodel.hpp"
#include <QColor>
#include <QDebug>
#include "theme.hpp"
#include "themes.hpp"

PreferenceModel::PreferenceModel(Theme *theme, QObject *parent)
    : QAbstractListModel(parent), theme_(theme), font_("Courier Prime", 12) {
  //  List column names that will appear in view
  roleNames_[CurrentCategoryRole] = "currentCategory";
  roleNames_[CurrentListRole] = "currentList";
  roleNames_[CurrentPrefRole] = "currentPreference";

  //  Load preferences
  load();

  //  Start with default preference
  current_ = theme_->preference(0);

  //  Set dirty flag anytime a preference or font changes
  QObject::connect(this, &PreferenceModel::preferenceChanged, theme, &Theme::setIsDirty);
  QObject::connect(this, &PreferenceModel::fontChanged, theme, &Theme::setIsDirty);

  QObject::connect(theme, &Theme::themesChanged, this, &PreferenceModel::onPreferenceRefreshed);
}

void PreferenceModel::onPreferenceRefreshed() { emit preferenceRefreshed(); }

QFont PreferenceModel::font() const { return font_; }

void PreferenceModel::setFont(QFont font) {
  beginResetModel();

  //  Update font
  theme_->setFont(font);

  endResetModel();
  emit fontChanged();
}

qint32 PreferenceModel::category() const { return category_; }

void PreferenceModel::setCategory(qint32 category) {
  beginResetModel();

  //  Update category
  category_ = category;

  endResetModel();
  emit categoryChanged();
}

QStringList PreferenceModel::categoryList() {

  emit categoryChanged();
  return categoryList_;
}

Preference *PreferenceModel::preference() const { return current_; }

void PreferenceModel::resetModel() {
  auto top = createIndex(0, 0);
  auto bottom = createIndex(rowCount(), 0);
  emit dataChanged(top, bottom);
}

void PreferenceModel::load() {
  //  Clear out list if load was called before
  categories_.clear();
  categoryList_.clear();

  auto &general = categories_.emplace_back("General");
  categoryList_.append(general.name());
  for (int i = Theme::Ranges::GeneralCategoryStart; i < Theme::Ranges::GeneralCategoryEnd; ++i) {
    general.addChild(theme_->preference(i)->name());
  }

  auto &editor = categories_.emplace_back("Editor");
  categoryList_.append(editor.name());
  for (int i = Theme::Ranges::GeneralCategoryEnd; i < Theme::Ranges::EditorCategoryEnd; ++i) {
    editor.addChild(theme_->preference(i)->name());
  }

  auto &circuit = categories_.emplace_back("Circuit");
  categoryList_.append(circuit.name());
  for (int i = Theme::Ranges::EditorCategoryEnd; i < Theme::Ranges::CircuitCategoryEnd; ++i) {
    circuit.addChild(theme_->preference(i)->name());
  }
}

int PreferenceModel::rowCount(const QModelIndex &parent) const {
  //  Rows depends on current parent
  return categories_.at(category_).size();
}

QVariant PreferenceModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return {};

  const auto row = index.row();

  //  See if current role is handled
  switch (role) {
  //  Name of curent category
  case RoleNames::CurrentCategoryRole: {
    const auto it = categories_.at(category_).preference(row);
    if (!it.isEmpty()) return it;
  } break;

  //  Return list of preferences. Each row is a separate preference
  case RoleNames::CurrentListRole: {
    //  This role is for iterating over list in for current selection
    //  Categories start with general and increment by 1
    //  Items under categories start at 100x of the category id

    int offset{};
    if (category_ == 1) offset = Theme::Ranges::EditorCategoryStart;
    else if (category_ == 2) offset = Theme::Ranges::CircuitCategoryStart;

    if (auto *pref = theme_->preference(row + offset); pref != nullptr) return QVariant::fromValue(pref);

  } break;

  //  Return currently selected preference
  //  Used for in preferences screen for overrides
  case RoleNames::CurrentPrefRole: {
    Q_ASSERT(current_);
    return QVariant::fromValue(current_);
  }
  }

  //  Role not found
  return {};
}

bool PreferenceModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  //  See if value is different from passed in value
  switch (role) {
    //  Update currently selected preference
  case RoleNames::CurrentPrefRole: {
    //  This is a copy. Use ID to find original
    Preference *temp = value.value<Preference *>();

    //  If preference hasn't changed, just exit
    if (temp->id() != current_->id()) {

      //  Update current preference
      current_ = temp;

      //  Signal that preference selected is different
      emit preferenceRefreshed();

      return true;
    }
  } break;
  }
  return false;
}

Qt::ItemFlags PreferenceModel::flags(const QModelIndex &index) const {
  const auto col = index.column();
  //  If field property, it is editable
  if (col == Qt::FontRole || col == CurrentPrefRole)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  //  All other fields are not editable
  return Qt::NoItemFlags;
}

QHash<int, QByteArray> PreferenceModel::roleNames() const { return roleNames_; }

void PreferenceModel::updatePreference(const quint32 key, const PrefProperty field, const QVariant &value) {
  //  Invalid key is bad
  if (key == Themes::Roles::Invalid) return;

  //  Roles are maintained in a vector. Lookup
  //  is relative to first role
  //  Get original
  auto *original = theme_->preference(key);
  switch (field) {
  case PrefProperty::Parent:
    //  No change, return
    if (original->parentId() == value.toInt()) return;

    //  Signal model change
    beginResetModel();
    original->setParent(value.toInt());
    break;
  case PrefProperty::Foreground: {
    QColor color = value.value<QColor>();
    //  No change, return
    if (original->foreground() == color) return;

    //  Signal model change
    beginResetModel();
    original->setForeground(color);
  } break;
  case PrefProperty::Background: {
    QColor color = value.value<QColor>();
    //  No change, return
    if (original->background() == color) return;

    //  Signal model change
    beginResetModel();
    original->setBackground(color);
  } break;
  case PrefProperty::Bold:
    //  No change, return
    if (original->bold() == value.toBool()) return;

    //  Signal model change
    beginResetModel();
    original->setBold(value.toBool());
    break;
  case PrefProperty::Italic:
    //  No change, return
    if (original->italics() == value.toBool()) return;

    //  Signal model change
    beginResetModel();
    original->setItalics(value.toBool());
    break;
  case PrefProperty::Underline:
    //  No change, return
    if (original->underline() == value.toBool()) return;

    //  Signal model change
    beginResetModel();
    original->setUnderline(value.toBool());
    break;
  case PrefProperty::Strikeout:
    //  No change, return
    if (original->strikeOut() == value.toBool()) return;

    //  Signal model change
    beginResetModel();
    original->setStrikeOut(value.toBool());
    break;

  default: return;
  }

  //  If we get here, model was updated. End reset.
  endResetModel();
  emit preferenceChanged();
  emit preferenceRefreshed();
}
