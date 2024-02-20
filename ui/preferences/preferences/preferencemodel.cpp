#include "preferencemodel.hpp"

//  For testing only
#include <QDebug>

PreferenceModel::PreferenceModel(QObject *parent)
    : QAbstractListModel(parent)
{
    //  List column names that will appear in view
    roleNames_[Categories]     = "categoriesRole";
    roleNames_[General]     = "generalCategoryRole";
    roleNames_[Editor]      = "editorCategoryRole";
    roleNames_[Alu]         = "aluCategoryRole";

    //  Individual properties
    roleNames_[Type]        = "typeRole";
    roleNames_[Foreground]  = "foregroundRole";
    roleNames_[Background]  = "backgroundRole";
    roleNames_[FontBold]    = "boldRole";
    roleNames_[FontItalics] = "italicsRole";
    roleNames_[FontUnderline]= "underlineRole";


    //  Load preferences
    load();
}

void PreferenceModel::load()
{
    //  List of elements is static
    preferences_.append(Preference(General, "General", Categories, 0,"#000000","#ffffff") );
    preferences_.append(Preference(Editor, "Editor", Categories, 0,"#000000","#ffffff") );
    preferences_.append(Preference(Alu, "Alu", Categories, 0,"#000000","#ffffff") );
}


/*void PreferenceModel::reload()
{
    //  List of elements is static
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    registers_.clear();
    load();
    endInsertRows();
}
*/

int PreferenceModel::rowCount(const QModelIndex &) const
{
    //  Number of preferences
    return preferences_.size();
}

QVariant PreferenceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto row = index.row();

    //  Is this a category request?
    if( role == Categories ) {
        const int lookup = Categories + row + 1;

        for(const auto& item : preferences_)  {
            if( lookup == item.id())
                return item.name();
        }
    }

    //  Get current status bit
    const auto pref = preferences_.at(row);
    switch(role) {
    case Id:
        // Return id of preference
        return pref.id();
    case Name:
        // Return preference name
        return pref.name();
    case Parent:
        // Return parent Id
        return pref.parentId();
    case Type:
        // Return type
        return pref.type();
    case Foreground:
        // Return Data in foreground color
        return pref.foreground();
    case Background:
        // Return Data in foreground color
        return pref.background();
    case FontBold:
        // Return Data in foreground color
        return pref.bold();
    case FontItalics:
        // Return Data in foreground color
        return pref.italics();
    case FontUnderline:
        // Return Data in foreground color
        return pref.underline();
    }

    //  Role not found
    return QVariant();
}

bool PreferenceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //  See if value is different from passed in value
    if (data(index, role) == value) {
        return false;
    }

    auto reg = preferences_.at(index.row());

    switch(role)
    {
    case RoleNames::Parent: {
        reg.setParent(value.toUInt());

        emit dataChanged(index, index, {role});
        return true;
    }
    case RoleNames::Foreground: {
        // Update data field
        const auto color = value.value<QColor>();
        reg.setForeground(color);
        emit dataChanged(index, index, {role});

        return true;
    }
    case RoleNames::Background: {
        // Update data field
        const auto color = value.value<QColor>();
        reg.setBackground(color);
        emit dataChanged(index, index, {role});

        return true;
    }
    }
    //  Ignore other flags
    return false;
}

Qt::ItemFlags PreferenceModel::flags(const QModelIndex &index) const
{
    const auto col = index.column();
    //  If field property, it is editable
    if(col <= RoleNames::FieldStart &&
       col >= RoleNames::FieldEnd)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    //  All other fields are not editable
    return Qt::NoItemFlags;
}

QHash<int, QByteArray> PreferenceModel::roleNames() const
{
    return roleNames_;
}
