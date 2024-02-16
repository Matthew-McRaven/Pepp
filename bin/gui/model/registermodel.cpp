#include "registermodel.h"

//  For testing only
#include <QRandomGenerator>

PepRegister::PepRegister(const QString name, const quint32 address, const QString data) :
    name_(name),
    address_(address),
    data_(data) {}

RegisterModel::RegisterModel(QObject *parent)
    : QAbstractListModel(parent)
{
    //  List column names that will appear in view
    roleNames_[Name]   = "nameRole";
    roleNames_[Address]= "addressRole";
    roleNames_[Data]   = "dataRole";

    //  Load registers
    load();
}

void RegisterModel::load()
{
    //  List of elements is static
    registers_.append(PepRegister("Accumulator",           0, "0") );
    registers_.append(PepRegister("Index Register",        0, "0") );
    registers_.append(PepRegister("Stack Pointer",         0, "0") );
    registers_.append(PepRegister("Program Counter",       0, "0") );
    registers_.append(PepRegister("Instruction Specifier", 0, "0") );
    registers_.append(PepRegister("Operand Specifier",     0, "") );
    registers_.append(PepRegister("Operand",               0, "") );
}

void RegisterModel::updateTestData()
{
    //  This function just creates random data to test that QML is updated when model is updated
    beginResetModel();
    for( auto& reg : registers_ )
    {
        const quint16 address = static_cast<quint16>( QRandomGenerator::global()->generate() );
        const QString data = QString( "%1").arg( QRandomGenerator::global()->generate() % 0xffff);
        reg.setAddress( address );
        reg.setData( data );
    }
    endResetModel();
}

void RegisterModel::reload()
{
    //  List of elements is static
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    registers_.clear();
    load();
    endInsertRows();
}

QVariant RegisterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( !roleNames_.contains(role))
        return QVariant();

    return roleNames_.value(role);
}

int RegisterModel::rowCount(const QModelIndex &) const
{
    //  Number of status bits for this CPU
    return registers_.size();
}

QVariant RegisterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto row = index.row();

    // Boundary check for the row
    if(row < 0 || row >= registers_.size()) {
        return QVariant();
    }

    //  Get current status bit
    const auto reg = registers_.at(row);
    switch(role) {
    case Name:
        // Return register name
        return reg.name();
    case Address:
        // Return value in register
        return reg.address();
    case Data:
        // Return Data in current register
        return reg.data();
    }

    //  Role not found
    return QVariant();
}

bool RegisterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //  See if value is different from passed in value
    if (data(index, role) == value) {
        return false;
    }

    auto reg = registers_.at(index.row());

    if(RoleNames::Address == role) {
        const quint16 addr = value.toUInt();
        reg.setAddress(addr);

        emit dataChanged(index, index, {role});
        return true;
    }
    else if( RoleNames::Data == role) {
        // Update data field
        const QString data = value.toString();
        reg.setData(data);
        emit dataChanged(index, index, {role});

        return true;
    }

    //  Ignore other flags
    return false;
}
/*
Qt::ItemFlags StatusBitModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable; // FIXME: Implement me!
}
*/
QHash<int, QByteArray> RegisterModel::roleNames() const
{
    return roleNames_;
}
