#include "preferencemodel.hpp"

//  For testing only
#include <QDebug>
#include <QColor>

PreferenceModel::PreferenceModel(QObject *parent)
    : QAbstractListModel(parent), font_("Courier New", 12)
{
    //  List column names that will appear in view
    roleNames_[CategoriesRole]      = "categories";
    roleNames_[CurrentCategoryRole] = "currentCategory";
    roleNames_[CurrentListRole]     = "currentList";
    roleNames_[CurrentPrefRole]     = "currentPreference";

    //  Add additional styles - TODO

    //  Load preferences
    load();
}

void PreferenceModel::load()
{
    //  Clear out list if load was called before
    categories_.clear();
    prefs_.clear();

    //  Set size to avoid reallocation
    prefs_.reserve(128);

    auto& general = categories_.emplaceBack("General");

    auto* pref = &prefs_.emplace_back(SurfaceRole,"Surface",
        Qt::black,qRgb(0xff,0xff,0xff));  //  Black/White
    pref->setFont(&font_);
    general.addPreference(pref->name());

           //  Save as current preference
    current_ = pref;

    pref = &prefs_.emplace_back(ContainerRole,"Container",
        qRgb(0x7f,0x7f,0x7f),qRgb(0xee,0xee,0xee));  //  Black/gray
    pref->setFont(&font_);
    general.addPreference(pref->name());

    pref = &prefs_.emplace_back(PrimaryRole,"Primary",
        qRgb(0x44,0x44,0x44),qRgb(0x90,0xeb,0xff), //  Black/Light Blue
        0, true);  // Bold
    pref->setFont(&font_);
    general.addPreference(pref->name());

    pref = &prefs_.emplace_back(SecondaryRole,"Secondary",
        qRgb(0xf0,0xf0,0xf0),qRgb(0x2e,0x3e,0x84), //  White/Dark Blue
        0, true, true); //  Bold, italics
    pref->setFont(&font_);
    general.addPreference(pref->name());

    pref = &prefs_.emplace_back(TertiaryRole,"Tertiary",
        qRgb(0xff,0xff,0xff),qRgb(0x35,0xb5,0x48), //  White/Green
        0, false, true, true);  //  Italics, underline
    pref->setFont(&font_);
    general.addPreference(pref->name());

    pref = &prefs_.emplace_back(ErrorRole,"Error",
        qRgb(0x00,0x00,0x00),qRgb(0xff,0x00,0x00), //  Black/Red
        0, true); //  Bold
    pref->setFont(&font_);
    general.addPreference(pref->name());

    pref = &prefs_.emplace_back(WarningRole,"Warning",
        qRgb(0x00,0x00,0x00),qRgb(0xff,0xaa,0x00), //  Black/Red
        0, false, false, false, true); // Strikeout
    pref->setFont(&font_);
    general.addPreference(pref->name());

    auto& editor  = categories_.emplaceBack("Editor");
    pref = &prefs_.emplace_back(RowNumberRole,"Row Number",
        qRgb(0x66,0x66,0x66),qRgb(0xff,0xff,0xff), //  Black/Red
        0, false, false, false, true); // Strikeout
    pref->setFont(&font_);
    editor.addPreference(pref->name());

    pref = &prefs_.emplace_back(BreakpointRole,"Breakpoint",
        qRgb(0x00,0x00,0x00),qRgb(0xff,0xaa,0x00), //  Black/Red
        0, false, false, false, true); // Strikeout
    pref->setFont(&font_);
    editor.addPreference(pref->name());

    auto& circuit = categories_.emplaceBack("Circuit");
    pref = &prefs_.emplace_back(SeqCircuitRole,"Breakpoint",
        qRgb(0xff,0xff,0x00),qRgb(0x04,0xab,0x0a), //  Yellow/Green
        0); // None
    pref->setFont(&font_);
    circuit.addPreference(pref->name());

    pref = &prefs_.emplace_back(CircuitGreenRole,"SeqCircuit",
        qRgb(0x0,0x0,0xff),qRgb(0xff,0xe1,0xff), //  Blue/Violet
        0); // None
    pref->setFont(&font_);
    circuit.addPreference(pref->name());
}

int PreferenceModel::rowCount(const QModelIndex &parent) const
{
    //  Rows depends on current parent
    return categories_.size() > categories_.at(category_).size()
        ? categories_.size()
        : categories_.at(category_).size();
}

QVariant PreferenceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto row = index.row();

    //  See if current role is handled
    switch(role)
    {
      //  Return list of categories. Each row is a different category
      case RoleNames::CategoriesRole: {
          return row < categories_.size()
                 ? categories_.at(row).name()
                 : QVariant();
      }
      //  Name of curent category
      case RoleNames::CurrentCategoryRole: {
        const auto it = categories_.at(category_).preference(row);
        if(!it.isEmpty())
            return it;
      }

      //  Return list of preferences. Each row is a separate preference
      case RoleNames::CurrentListRole: {
        //  This role is for iterating over list in for current selection
        //  Categories start with general and increment by 1
        //  Items under categories start at 100x of the category id

        int offset{};
        if(category_ == 1)
          offset = RowNumberRole - SurfaceRole;
        else if(category_ == 2)
          offset = SeqCircuitRole - SurfaceRole;

        if( offset + row >= prefs_.size())
          return {};

        auto& pref = prefs_[row + offset];
        return QVariant::fromValue(pref);
      }

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

bool PreferenceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

    //  See if value is different from passed in value
    //if (data(index, role) == value) {
    //    return false;
    //}

    //  See if value is different from passed in value
    switch (role) {
      //  Update currently selected preference
      case RoleNames::CurrentPrefRole:
        //  This is a copy. Use ID to find original
        Preference temp = value.value<Preference>();

        //  If preference hasn't changed, just exit
        if( temp.id() == current_->id())
          return false;

        if(category_ == 0) {
          //  Update current preference
          current_ = &prefs_[temp.id()- RoleNames::SurfaceRole];
          //preference_ = temp.id();
          emit preferenceChanged();

          return true;
        }

        /*if(preferences_.contains(temp.id())) {

          //  Update current preference
          preference_ = temp.id();
          emit preferenceChanged();

          return true;
        }*/
    }
    return false;
}

Qt::ItemFlags PreferenceModel::flags(const QModelIndex &index) const
{
    const auto col = index.column();
    //  If field property, it is editable
    if(col == Qt::FontRole ||
        col >= RoleNames::GeneralRole)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    //  All other fields are not editable
    return Qt::NoItemFlags;
}

QHash<int, QByteArray> PreferenceModel::roleNames() const
{
    return roleNames_;
}

void PreferenceModel::updatePreference(const quint32 key,
                      const PrefProperty field,
                      const QVariant& value)
{
  if( key >= RoleNames::RowNumberRole)
    return;

  //  Roles are maintained in a vector. Lookup
  //  is relative to first role
  auto lookup = key - RoleNames::SurfaceRole;
  //  Get original
  //if(preferences_.contains(key)) {
    //auto original = preferences_.value(key);
    auto original = prefs_[lookup];
    switch(field) {
      case PrefProperty::Parent:
        //  No change, return
        if(original.parentId() == value.toInt()) return;

        //  Signal model change
        beginResetModel();
        original.setParent(value.toInt());
        break;
      case PrefProperty::Foreground: {
        QColor color = value.value<QColor>();
        //  No change, return
        if(original.foreground() == color) return;

        //  Signal model change
        beginResetModel();
        original.setForeground(color);
      }
      break;
      case PrefProperty::Background: {
        QColor color = value.value<QColor>();
        //  No change, return
        if(original.background() == color) return;

        //  Signal model change
        beginResetModel();
        original.setBackground(color);
      }
      break;
      case PrefProperty::Bold:
        //  No change, return
        if(original.bold() == value.toBool()) return;

        //  Signal model change
        beginResetModel();
        original.setBold(value.toBool());
        break;
      case PrefProperty::Italic:
        //  No change, return
        if(original.italics() == value.toBool()) return;

        //  Signal model change
        beginResetModel();
        original.setItalics(value.toBool());
        break;
      case PrefProperty::Underline:
        //  No change, return
        if(original.underline() == value.toBool()) return;

        //  Signal model change
        beginResetModel();
        original.setUnderline(value.toBool());
        break;
      case PrefProperty::Strikeout:
        //  No change, return
        if(original.strikeOut() == value.toBool()) return;

        //  Signal model change
        beginResetModel();
        original.setStrikeOut(value.toBool());
        break;

      default:
        return;
    }

    //  If we get here, model was updated. End reset.
    endResetModel();
    emit preferenceChanged();
  //}
}
