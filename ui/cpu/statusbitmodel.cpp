#include "statusbitmodel.h"

StatusBit::StatusBit(const QString statusBit, const bool flag ) :
    statusBit_(statusBit),
    flag_(flag) {}

StatusBitModel::StatusBitModel(QObject *parent)
    : QAbstractListModel(parent)
{
    //  List column names that will appear in view
    roleNames_[Status]   = "statusBitRole";
    roleNames_[Flag]     = "flagRole";

    load();
}

void StatusBitModel::load()
{
    //  List of elements is static
    statusBits_.append(StatusBit("N", false) );
    statusBits_.append(StatusBit("Z", false) );
    statusBits_.append(StatusBit("V", false) );
    statusBits_.append(StatusBit("C", false) );
}

void StatusBitModel::updateTestData()
{
    //  This function just creates random data to test that QML is updated when model is updated
    beginResetModel();
    for( auto& bit : statusBits_ )
    {
        bit.setFlag( (QRandomGenerator::global()->generate() % 2 ) == 0);
    }
    endResetModel();
}

void StatusBitModel::reload()
{
    //  List of elements is static
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    statusBits_.clear();
    load();
    endInsertRows();
}

QVariant StatusBitModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( !roleNames_.contains(role))
        return QVariant();

    return roleNames_.value(role);
}

int StatusBitModel::rowCount(const QModelIndex &) const
{
    //  Number of status bits for this CPU
    return statusBits_.size();
}

QVariant StatusBitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto row = index.row();

    // Boundary check for the row
    if(row < 0 || row >= statusBits_.size()) {
        return QVariant();
    }

    //  Get current status bit
    const auto statusBit = statusBits_.at(row);
    switch(role) {
    case Status:
        // Status bit indicator
        return statusBit.statusBit();
    case Flag:
        // Return current state for status bit
        return statusBit.flag();
    }

    //  Role not found
    return QVariant();
}

bool StatusBitModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //  See if value is different from passed in value
    if (data(index, role) == value) {
        return false;
    }

    const auto row = index.row();

    switch(role) {
    case Status:
        // Status indicators are not changing during execution
        break;
    case Flag:
        // Set flag status
        const bool flag = value.toBool();
        statusBits_[row].setFlag(flag);
        emit dataChanged(index, index, {role});

        return true;
    }
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
QHash<int, QByteArray> StatusBitModel::roleNames() const
{
    return roleNames_;
}
