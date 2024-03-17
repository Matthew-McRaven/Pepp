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

    //  Basic text styles
    roleNames_[NormalTextRole]          = "normalText";
    roleNames_[BackgroundRole]          = "backgroundText";
    roleNames_[SelectionRole]           = "selectedText";
    roleNames_[Test1Role]               = "pepperdineText";
    //  Add additional styles - TODO

    //  Load preferences
    load();
}

void PreferenceModel::load()
{
    //  Clear out list if load was called before
    categories_.clear();

    auto& general = categories_.emplaceBack("General");

    auto it = preferences_.emplace(NormalTextRole,NormalTextRole,"Foreground");
    it->setForeground(Qt::black);
    it->setBackground(qRgb(0xff,0xff,0xff));    //  White
    it->setFont(&font_);
    general.addPreference(it->name());

    it = preferences_.emplace(BackgroundRole,BackgroundRole,"Background");
    it->setForeground(Qt::black);
    it->setBackground(qRgb(0xb5,0xb5,0xb5));    //  Light gray
    general.addPreference(it->name());

    it = preferences_.emplace(SelectionRole,SelectionRole,"Selection");
    it->setForeground(qRgb(0x44,0x44,0x44));
    it->setBackground(qRgb(0x90,0xeb,0xff));    //  Light blue
    it->setBold(true);
    general.addPreference(it->name());

    it = preferences_.emplace(Test1Role,Test1Role,"Go Pepperdine!");
    it->setForeground(qRgb(0xff,0xaa,0x00));    //  Orange
    it->setBackground(qRgb(0x3f,0x51,0xb5));    //  Blue
    it->setBold(true);
    it->setItalics(true);
    general.addPreference(it->name());

    auto& editor  = categories_.emplaceBack("Editor");
    it = preferences_.emplace(RowNumberRole,RowNumberRole,"Row Number");
    it->setForeground(qRgb(0x66,0x66,0x66));    //  Dark Gray
    it->setBackground(qRgb(0xff,0xff,0xff));    //  White
    it->setStrikeOut(true);
    editor.addPreference(it->name());
    it = preferences_.emplace(BreakpointRole,BreakpointRole,"Breakpoint");
    it->setForeground(qRgb(0xff,0x0,0x0));      //  Red
    it->setBackground(qRgb(0xff,0xff,0xff));    //  White
    it->setUnderline(true);
    editor.addPreference(it->name());

    auto& circuit = categories_.emplaceBack("Circuit");
    it = preferences_.emplace(SeqCircuitRole,SeqCircuitRole,"SeqCircuit");
    it->setForeground(qRgb(0xff,0xff,0x0));     //  Yellow
    it->setBackground(qRgb(0x04,0xab,0x0a));    //  Green
    circuit.addPreference(it->name());
    it = preferences_.emplace(CircuitGreenRole,CircuitGreenRole,"CircuitGreen");
    it->setForeground(qRgb(0x0,0x0,0xff));      //  Blue
    it->setBackground(qRgb(0xff,0xe1,0xff));    //  Violet
    circuit.addPreference(it->name());
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
        const int current = (GeneralRole + category_) * 100 + row;

        if(preferences_.contains(current)) {
            auto& pref = preferences_[current];

            return QVariant::fromValue(pref);
        }
      }

      //  Return currently selected preference
      //  Used for in preferences screen for overrides
      case RoleNames::CurrentPrefRole:
        return QVariant::fromValue(preference());

      //  Specific preference based on role
      case RoleNames::NormalTextRole:
      case RoleNames::BackgroundRole:
      case RoleNames::SelectionRole:
      case RoleNames::Test1Role:
      {
        qDebug() << "Called text selection" << role;
        if(preferences_.contains(role)) {
            auto& pref = preferences_[role];

            return QVariant::fromValue(pref);
        }
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
        if( temp.id() == preference_)
          return false;

        if(preferences_.contains(temp.id())) {

          //  Update current preference
          preference_ = temp.id();
          emit preferenceChanged();

          return true;
        }
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
  //  Get original
  if(preferences_.contains(key)) {
    auto original = preferences_.value(key);
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
  }

}
