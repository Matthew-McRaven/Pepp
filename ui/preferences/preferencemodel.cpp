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

    //  Basic text styles
    roleNames_[NormalText]          = "normalText";
    //roleNames_[Selection]           = "selected";
    //  Add additional styles - TODO

    //  Load preferences
    load();
}

void PreferenceModel::load()
{
    //  Clear out list if load was called before
    categories_.clear();

    auto& general = categories_.emplaceBack("General");

    auto it = preferences_.emplace(NormalText,NormalText,"Foreground",General);
    it->setForeground(Qt::black);
    it->setBackground(qRgb(0xff,0xff,0xff));    //  White
    it->setFont(&font_);
    general.addPreference(it->name());

    it = preferences_.emplace(Background,Background,"Background",General);
    it->setForeground(Qt::black);
    it->setBackground(qRgb(0xb5,0xb5,0xb5));    //  Light gray
    general.addPreference(it->name());

    it = preferences_.emplace(Selection,Selection,"Selection",General);
    it->setForeground(qRgb(0x44,0x44,0x44));
    it->setBackground(qRgb(0x90,0xeb,0xff));    //  Light blue
    it->setBold(true);
    general.addPreference(it->name());

    it = preferences_.emplace(Test1,Test1,"Go Pepperdine!",General);
    it->setForeground(qRgb(0xff,0xaa,0x00));    //  Orange
    it->setBackground(qRgb(0x3f,0x51,0xb5));    //  Blue
    it->setBold(true);
    it->setItalics(true);
    general.addPreference(it->name());

    auto& editor  = categories_.emplaceBack("Editor");
    it = preferences_.emplace(RowNumber,RowNumber,"Row Number",Editor);
    it->setForeground(qRgb(0x66,0x66,0x66));    //  Dark Gray
    it->setBackground(qRgb(0xff,0xff,0xff));    //  White
    it->setStrikeOut(true);
    editor.addPreference(it->name());
    it = preferences_.emplace(Breakpoint,Breakpoint,"Breakpoint",Editor);
    it->setForeground(qRgb(0xff,0x0,0x0));      //  Red
    it->setBackground(qRgb(0xff,0xff,0xff));    //  White
    it->setUnderline(true);
    editor.addPreference(it->name());

    auto& circuit = categories_.emplaceBack("Circuit");
    it = preferences_.emplace(SeqCircuit,SeqCircuit,"SeqCircuit",Editor);
    it->setForeground(qRgb(0xff,0xff,0x0));     //  Yellow
    it->setBackground(qRgb(0x04,0xab,0x0a));    //  Green
    circuit.addPreference(it->name());
    it = preferences_.emplace(CircuitGreen,CircuitGreen,"CircuitGreen",Editor);
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

    //  If row is greater than preferences, just return
    switch(role)
    {
    case RoleNames::CategoriesRole:
        return row < categories_.size()
               ? categories_.at(row).name()
               : QVariant();
    case RoleNames::CurrentCategoryRole: {
        const auto it = categories_.at(category_).preference(row);
        if(!it.isEmpty())
            return it;

        return {};
    }

    case RoleNames::CurrentListRole: {
        //  This role is for iterating over list in for current selection
        //  Categories start with general and increment by 1
        //  Items under categories start at 100x of the category id
        const int current = (General + category_) * 100 + row;

        if(preferences_.contains(current)) {
            auto& pref = preferences_[current];

            return QVariant::fromValue(pref);
        }

        //  Not found
        return {};
    }
    case RoleNames::NormalText: {
        if(preferences_.contains(role)) {
            auto& pref = preferences_[role];

            return QVariant::fromValue(pref);
        }

        //  Not found
        return {};
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
    //switch (role) {
    //  Standard Qt roles
    //}
    return false;
}

Qt::ItemFlags PreferenceModel::flags(const QModelIndex &index) const
{
    const auto col = index.column();
    //  If field property, it is editable
    if(col == Qt::FontRole ||
        col >= RoleNames::General)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    //  All other fields are not editable
    return Qt::NoItemFlags;
}

QHash<int, QByteArray> PreferenceModel::roleNames() const
{
    return roleNames_;
}
