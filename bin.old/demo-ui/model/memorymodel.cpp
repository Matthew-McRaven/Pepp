#include "memorymodel.h"

//  For testing only
#include <QRandomGenerator>


/*Register::Register(const quint32 address, const QString data) :
    address_(address),
    data_(data) {}*/

MemoryModel::MemoryModel(QObject *parent)
    : QAbstractListModel(parent),
    data_( 1 << 8, 0U)
{
    roleNames_[Byte]        = "byteRole";
    roleNames_[Character]   = "characterRole";
}

QHash<int, QByteArray> MemoryModel::roleNames() const
{
    return roleNames_;
}

void MemoryModel::updateTestData()
{
    //  This function just creates random data to test that QML is updated when model is updated
    //beginResetModel();

    quint8 data{};
    QModelIndex qi;
    //  Update first 8 bytes with random data
    for( int i = 0; i < 8; ++i)
    {
        qi = QAbstractItemModel::createIndex(i, 0);
        data = static_cast<quint8>( QRandomGenerator::global()->generate() );
        data_[i] = data;
        emit dataChanged(qi, qi, {Qt::DisplayRole, Qt::EditRole});
    }
    //endResetModel();
}
QVariant MemoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    //  No header data
    //if( roleNames_.contains( role ) ) {
    //    return roleNames_.value(role);
    //}

    return QVariant();
}

int MemoryModel::rowCount(const QModelIndex &parent) const
{
    //  Number of CPU registers
    return data_.size();
}

QVariant MemoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // the index returns the requested row and column information
    // we ignore the column and only use the row information
    const int row = index.row();

    // boundary check for the row
    if(row < 0 || row >= data_.size()) {
        return QVariant();
    }

    switch(role) {
    case Character:  {
        QChar c(QString::number(data_.at(row)).at(0));
        if( c.isPunct() ) {
            return c;
        } else {
            return QString(".");
        }
        break;
    }
    case Byte:
        return data_.at(row);
    case Qt::DisplayRole:
        return data_.at(row);
    case Qt::ForegroundRole:
        break;
    case Qt::BackgroundRole:
        break;
    }

    return QVariant();
}

bool MemoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //  See if value is different from passed in value
    if (data(index, role) == value) {
        emit dataChanged(index, index, {role});
        return false;
    }

    const auto row = index.row();

    if(Qt::EditRole == role) {
        const auto mem = value.toChar().unicode();
        data_[row] = mem;

        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}
