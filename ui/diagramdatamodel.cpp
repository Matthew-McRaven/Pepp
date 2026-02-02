#include "diagramdatamodel.hpp"

DiagramDataModel::DiagramDataModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

void DiagramDataModel::update(int row, int column)
{
    const QModelIndex index = this->index(row, column);
    emit dataChanged(index, index); //, {spread::Role::Display,});
}

bool DiagramDataModel::clearItemData(const QModelIndexList &indexes)
{
    bool ok = true;
    for (const QModelIndex &index : indexes)
        ok &= clearItemData(index);
    return ok;
}

bool DiagramDataModel::clearItemData(const QModelIndex &index)
{
    if (!index.isValid())
        return false;
    if (_data.clearData(DiagramKey{index.row(), index.column()})) {
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

DiagramProperties *DiagramDataModel::itemData(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    auto data = _data.getDiagramProps(DiagramKey{index.row(), index.column()});
    return data;
}

/*
QVariant DiagramDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
}

bool DiagramDataModel::setHeaderData(int section,
                                     Qt::Orientation orientation,
                                     const QVariant &value,
                                     int role)
{
    if (value != headerData(section, orientation, role)) {
        // FIXME: Implement me!
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}
*/
int DiagramDataModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _rowSize;
}

int DiagramDataModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _colSize;
}

QVariant DiagramDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const auto row = index.row();
    const auto col = index.column();

    return _data.getData(DiagramKey{row, col}, role);
}

bool DiagramDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    const int col = index.column();

    if (_data.setData(DiagramKey{row, col}, value, role))
        emit dataChanged(index, index);

    return true;
}

Qt::ItemFlags DiagramDataModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

/*bool DiagramDataModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();
    return true;
}

bool DiagramDataModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endInsertColumns();
    return true;
}

bool DiagramDataModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();
    return true;
}

bool DiagramDataModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    beginRemoveColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endRemoveColumns();
    return true;
}
*/

QHash<int, QByteArray> DiagramDataModel::roleNames() const
{
    return {{DiagramProperty::Role::Name, "name"},
            {DiagramProperty::Role::Id, "id"},
            {DiagramProperty::Role::ImageSource, "imageSource"},
            {DiagramProperty::Role::Type, "diagramType"},
            {DiagramProperty::Role::InputNo, "inputNo"},
            {DiagramProperty::Role::OutputNo, "outputNo"}};
}
