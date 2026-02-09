#include "diagramdatamodel.hpp"

DiagramDataModel::DiagramDataModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

void DiagramDataModel::update(int row, int column)
{
    update(this->index(row, column));
}

void DiagramDataModel::update(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    emit dataChanged(index, index);
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

const QModelIndex DiagramDataModel::currentItem() const
{
    return _current;
}

DiagramProperties *DiagramDataModel::item(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    auto data = _data.getDiagramProps(DiagramKey{index.row(), index.column()});
    return data;
}

DiagramProperties *DiagramDataModel::createItem(int row, int column)
{
    return createItem(this->index(row, column));
}

DiagramProperties *DiagramDataModel::createItem(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    auto data = _data.createDiagramProps(DiagramKey{index.row(), index.column()});
    emit dataChanged(index, index);
    return data;
}

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

QHash<int, QByteArray> DiagramDataModel::roleNames() const
{
    return {{DiagramProperty::Role::Name, "name"},
            {DiagramProperty::Role::Id, "id"},
            {DiagramProperty::Role::ImageSource, "imageSource"},
            {DiagramProperty::Role::Type, "diagramType"},
            {DiagramProperty::Role::InputNo, "inputNo"},
            {DiagramProperty::Role::OutputNo, "outputNo"}};
}
