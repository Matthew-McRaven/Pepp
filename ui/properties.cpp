#include "properties.h"

quint32 DiagramProperty::_counter = 0;

DiagramProperty::DiagramProperty(QObject *parent)
    : QObject(parent)
    , _id{++_counter}
{}

void DiagramProperty::setName(const QString v)
{
    if (_name != v) {
        _name = v;
        emit nameChanged();
    }
}

void DiagramProperty::setImageSource(const QString v)
{
    if (_imageSrc != v) {
        _imageSrc = v;
        emit imageChanged();
    }
}

void DiagramProperty::setType(const int v)
{
    if (_type != static_cast<DiagramRole>(v)) {
        _type = static_cast<DiagramRole>(v);
        emit typeChanged();
    }
}

void DiagramProperty::setInputNo(const quint16 v)
{
    if (_inputNo != v) {
        _inputNo = v;
        emit inputChanged();
    }
}

void DiagramProperty::setOutputNo(const quint16 v)
{
    if (_outputNo != v) {
        _outputNo = v;
        emit outputChanged();
    }
}

DiagramPropertyModel::DiagramPropertyModel(QObject *parent)
    : QAbstractListModel(parent)
{}

DiagramProperty *DiagramPropertyModel::createDiagram()
{
    //  Create item with model as parent for lifetime management
    _currentProperty = new DiagramProperty(this);

    //beginInsertRows(QModelIndex{}, _properties.count() + 1, _properties.count() + 1);
    _properties.append(_currentProperty);
    //endInsertRows();

    emit newDiagram();
    emit diagramChanged();

    return _currentProperty;
}

DiagramProperty *DiagramPropertyModel::currentDiagram() const
{
    return _currentProperty;
}
void DiagramPropertyModel::setCurrentDiagram(DiagramProperty *v)
{
    if (_currentProperty != v) {
        _currentProperty = v;
        emit diagramChanged();
    }
}

int DiagramPropertyModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    return _properties.size();
}

int DiagramPropertyModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    return static_cast<int>(DiagramProperty::DiagramRole::DiagramTotal);
}

QVariant DiagramPropertyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const auto row = index.row();

    //  Check for invalid row
    if (row < 0 || _properties.size() < row)
        return {};

    const auto *item = _properties.at(index.row());

    switch (index.column()) {
    case 0:
        return item->id();
        break;
    case 1:
        return item->name();
        break;
    case 2:
        return item->type();
        break;
    case 3:
        return item->inputNo();
        break;
    case 4:
        return item->outputNo();
        break;
    }

    return {};
}

/*bool DiagramPropertyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        const auto row = index.row();

        //  Check for invalid row
        if (row < 0 || _properties.size() < row)
            return false;

        //  Get editable item
        auto *item = _properties[index.row()];
        bool ok = false;

        switch (index.column()) {
        case 1:
            item->setName(value.toString());
            break;
        case 2: {
            auto val = value.toInt(&ok);

            //  Did conversion fail?
            if (!ok)
                return false;

            item->setType(val);
            break;
        }
        case 3: {
            auto val = value.toUInt(&ok);

            //  Did conversion fail?
            if (!ok)
                return false;

            item->setInputNo(val);
            break;
        }
        case 4: {
            auto val = value.toUInt(&ok);

            //  Did conversion fail?
            if (!ok)
                return false;

            item->setOutputNo(val);
            break;
        }
        //  Invalid property
        default:
            return false;
        }
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}*/

Qt::ItemFlags DiagramPropertyModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

/*bool DiagramPropertyModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    auto ok = (nullptr != createDiagram());
    endInsertRows();
    return ok;
}

bool DiagramPropertyModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();
    return true;
}
*/
