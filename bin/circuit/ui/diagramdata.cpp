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
        return nullptr;

    return it_cell.value();
}

DiagramProperties *DiagramData::createDiagramProps(const DiagramKey &key)
{
    //  See if something already exists at this location
    DiagramProperties *cell = getDiagramProps(key);
    if (cell != nullptr)
        return cell;

    //  Doesn't exist, create now
    // cell = new DiagramProperties();

    //  Any error creating?
    // if (cell == nullptr)
    //    return nullptr;

    auto &ref = _data.emplace_back();
    _cells.insert(key, &ref);
    _keys.insert(ref.id(), key);

    return &ref;
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

void DiagramData::moveData(const DiagramKey &oldKey, const DiagramKey &newKey)
{
    auto *cell = getDiagramProps(oldKey);

    if (cell == nullptr)
        return;

    //  Insert into new
    _cells.insert(newKey, cell);
    _keys.insert(cell->id(), newKey);

    //  Remove old key
    _cells.remove(oldKey);
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
