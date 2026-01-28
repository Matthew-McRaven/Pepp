#include "diagramModel.hpp"

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
    _properties.insert(_currentProperty->id(), _currentProperty);
    //endInsertRows();

    emit newDiagram();
    emit diagramChanged();

    return _currentProperty;
}

/*DiagramProperty *DiagramPropertyModel::currentDiagram() const
{
    return _currentProperty;
}
void DiagramPropertyModel::setCurrentDiagram(DiagramProperty *v)
{
    if (_currentProperty != v) {
        _currentProperty = v;
        emit diagramChanged();
    }
}*/

/*bool DiagramPropertyModel::selected(quint32 id) const
{
    return _currentProperty == nullptr ? false : _currentProperty->id() == id;
}

void DiagramPropertyModel::setSelected(quint32 id)
{
    const auto oldId = _currentProperty->id();
    if (oldId != id) {
        auto item = _properties.find(id);

        if (item == _properties.constEnd()) {
            //  Not found, clear current property
            _currentProperty = nullptr;
        } else {
            _currentProperty = item.value();

            //  Select new index
            auto newIndex = createIndex(_currentProperty->id(), 0);
            emit dataChanged(newIndex, newIndex);
        }

        //  Used to unselect old index
        auto oldIndex = createIndex(oldId, 0);
        emit dataChanged(oldIndex, oldIndex);
    }
}*/

int DiagramPropertyModel::rowCount(const QModelIndex &parent) const
{
    /*if (!parent.isValid())
        return 0;*/

    return _properties.size();
}

int DiagramPropertyModel::columnCount(const QModelIndex &parent) const
{
    /*if (!parent.isValid())
        return 0;*/

    return static_cast<int>(DiagramProperty::DiagramRole::DiagramTotal);
}

/*QModelIndex DiagramPropertyModel::index(int row,
                                        int column,
                                        const QModelIndex &parent) const override
{
    const auto item = _properties.find(row);
    if (item != _properties.constEnd()) {
        return QModelIndex(item.key(), 0, nullptr, this);
    }

    return {};
}*/

QVariant DiagramPropertyModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const auto row = index.row();

    //  Check for invalid row
    if (row < 0 || _properties.size() < row)
        return {};

    const auto item = _properties.find(row);

    if (item == _properties.constEnd()) {
        //  Not found, clear current property
        return {};
    }
    //const auto *item = _properties.at(index.row());

    switch (index.column()) {
    case 0:
        return item.value()->id();
        break;
    case 1:
        return item.value()->name();
        break;
    case 2:
        return item.value()->type();
        break;
    case 3:
        return item.value()->inputNo();
        break;
    case 4:
        return item.value()->outputNo();
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

/*  
 * Start of selection model
 */
void DiagramSelectionModel::setBehavior(Behavior behavior)
{
    if (behavior == _behavior)
        return;
    _behavior = behavior;
    emit behaviorChanged();
}
