#include "diagramdatamodel.hpp"

DiagramDataModel::DiagramDataModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

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

const QModelIndex DiagramDataModel::currentIndex() const
{
    return _current;
}

void DiagramDataModel::setCurrentIndex(const QModelIndex v)
{
    if (v != _current) {
        _current = v;
        emit currentIndexChanged();
    }
}

DiagramProperties *DiagramDataModel::item(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    auto data = _data.getDiagramProps(DiagramKey{index.row(), index.column()});
    return data;
}

DiagramProperties *DiagramDataModel::createItem(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;

    auto data = _data.createDiagramProps(DiagramKey{index.row(), index.column()});
    emit dataChanged(index, index);
    return data;
}

QModelIndex DiagramDataModel::index(int row, int column, const QModelIndex &parent) const
{
    // Check if row and column are within bounds and parent is invalid
    if (!hasIndex(row, column, parent))
        return {};

    // The internalPointer can store a pointer to your underlying data for quick access in data()
    // For a simple list, we can just use the row and column
    return createIndex(row, column, nullptr); // Use 0 or another pointer if you use internal data
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

    //  Set currently selected if value is true
    if (role == DiagramProperty::Role::Selected && value.toBool()) {
        setCurrentIndex(index);
        emit dataChanged(index, index);
    }

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
