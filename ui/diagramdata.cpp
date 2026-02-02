#include <QVariant>

#include "diagramdata.hpp"
#include "diagramproperty.hpp"

DiagramData::DiagramData() {}

bool DiagramData::empty() const
{
    return _keys.empty();
}

QVariant DiagramData::getData(int id, int role) const
{
    auto it_key = _keys.find(id);
    if (it_key == _keys.end())
        return {};
    const DiagramKey &key = it_key.value();
    auto it_cell = _cells.find(key);
    if (it_cell == _cells.end())
        return {};
    return it_cell.value()->get(role);
}

QVariant DiagramData::getData(const DiagramKey &key, int role) const
{
    auto it = _cells.find(key);
    return it == _cells.end() ? QVariant{} : it.value()->get(role);
}

bool DiagramData::setData(const DiagramKey &key, const QVariant &value, int role)
{
    // See if data already exists, and update
    if (auto it = _cells.find(key); it != _cells.end()) {
        it.value()->set(role, value);

    } else {
        //  If new item, but value is empty, just return.
        if (value.isNull())
            return false;

        //  New cell, create data structure
        DiagramProperties *cell = createDiagramProps(key);

        if (cell == nullptr)
            return false;

        cell->set(role, value);
    }

    return true;
}

DiagramProperties *DiagramData::getDiagramProps(const DiagramKey &key)
{
    auto it_cell = _cells.find(key);
    if (it_cell == _cells.end())
        return {};

    return it_cell.value();
}

DiagramProperties *DiagramData::createDiagramProps(const DiagramKey &key)
{
    DiagramProperties *cell = new DiagramProperties(this);
    if (cell == nullptr)
        return nullptr;

    _cells.insert(key, cell);
    _keys.insert(cell->id(), key);

    return cell;
}

bool DiagramData::clearData(const DiagramKey &key)
{
    auto find_key = [&key](const auto &i) { return i == key; };
    auto it = std::find_if(_keys.cbegin(), _keys.cend(), find_key);
    if (it == _keys.cend())
        return false;
    _keys.erase(it);
    return _cells.remove(key) > 0;
}

int DiagramData::createId(const DiagramKey &key)
{
    auto find_key = [&key](const auto &elem) { return key == elem; };
    auto it = std::find_if(_keys.begin(), _keys.end(), find_key);
    if (it != _keys.end())
        return it.key();

    //  Key not found, create now
    auto props = createDiagramProps(key);
    if (props == nullptr)
        //  Error in creation, return dummy number
        return -1;

    //  Return new number
    return props->id();
}

int DiagramData::getId(const DiagramKey &key) const
{
    auto find_key = [&key](const auto &elem) { return key == elem; };
    auto it = std::find_if(_keys.begin(), _keys.end(), find_key);
    if (it == _keys.end())
        return 0;
    return it.key();
}

DiagramKey DiagramData::getKey(int id) const
{
    auto it = _keys.find(id);
    return it == _keys.end() ? DiagramKey{-1, -1} : it.value();
}
