#include "diagramlistmodel.hpp"
#include <QImage>

DiagramTemplate::DiagramTemplate(QObject *parent)
    : QObject(parent) {};
DiagramTemplate::DiagramTemplate(
    DiagramType::Type key, QString name, QString type, QString qrc, QString file, QObject *parent)
    : QObject(parent)
    , _key(key)
    , _name(name)
    , _diagramType(type)
    , _qrcFile(qrc)
    , _file(file) {};

DiagramListModel::DiagramListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _diagrams.append(new DiagramTemplate(DiagramType::Invalid,
                                         "Move",
                                         "Arrow",
                                         "qrc:/move",
                                         "svg/move-arrow.svg",
                                         this));
    _diagrams.append(new DiagramTemplate(DiagramType::ANDGate,
                                         "AND Gate",
                                         "Diagram",
                                         "qrc:/and",
                                         "svg/and.svg",
                                         this));

    _diagrams.append(new DiagramTemplate(DiagramType::ORGate,
                                         "OR Gate",
                                         "Diagram",
                                         "qrc:/or",
                                         "svg/or.svg",
                                         this));
    _diagrams.append(new DiagramTemplate(DiagramType::Inverter,
                                         "Inverter",
                                         "Diagram",
                                         "qrc:/inverter",
                                         "svg/inverter.svg",
                                         this));
    _diagrams.append(new DiagramTemplate(DiagramType::NANDGate,
                                         "NAND Gate",
                                         "Diagram",
                                         "qrc:/nand",
                                         "svg/nand.svg",
                                         this));
    _diagrams.append(new DiagramTemplate(DiagramType::NORGate,
                                         "NOR Gate",
                                         "Diagram",
                                         "qrc:/nor",
                                         "svg/nor.svg",
                                         this));
    _diagrams.append(new DiagramTemplate(DiagramType::XORGate,
                                         "XOR Gate",
                                         "Diagram",
                                         "qrc:/xor",
                                         "svg/xor.svg",
                                         this));
    _diagrams.append(
        new DiagramTemplate(DiagramType::Line, "Line", "Line", "qrc:/line", "svg/line.svg", this));
    _diagrams.append(new DiagramTemplate(DiagramType::MultiLine,
                                         "Multiline",
                                         "Line",
                                         "qrc:/multiline",
                                         "svg/multiline.svg",
                                         this));
    _diagrams.append(
        new DiagramTemplate(DiagramType::Bus, "Bus", "Line", "qrc:/bus", "svg/bus.svg", this));
}

int DiagramListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return _diagrams.size();
}

QVariant DiagramListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const auto *item = this->diagramTemplate(index.row()); //_diagrams.at(index.row());

    switch (role) {
    case Role::Key:
        return item->key();
    case Role::Name:
        return item->name();
    case Role::DiagramType:
        return item->diagramType();
    case Role::QrcFile:
        return item->qrcFile();
    case Role::File:
        return item->file();
    }

    //  property not found
    return {};
}

QHash<int, QByteArray> DiagramListModel::roleNames() const
{
    return {{Role::Name, "name"},
            {Role::Key, "key"},
            {Role::DiagramType, "shapeType"},
            {Role::File, "file"},
            {Role::QrcFile, "qrcFile"}};
}

DiagramTemplate *DiagramListModel::diagramTemplate(int index) const
{
    if (0 <= index && index < _diagrams.size()) {
        return _diagrams.at(index);
    }

    //  Index is invalid rate
    return nullptr;
}

FilterDiagramListModel::FilterDiagramListModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortRole(Qt::DisplayRole);
}

void FilterDiagramListModel::setFilterGroupFilter(Filter filter)
{
    if (_filter != filter) {
        beginFilterChange();
        _filter = filter;

        switch (_filter) {
        case Arrow:
            _filterString = "Arrow";
            break;
        case Diagram:
            _filterString = "Diagram";
            break;
        case Line:
            _filterString = "Line";
            break;
        default:
            _filterString.clear();
        }

        endFilterChange(QSortFilterProxyModel::Direction::Rows);
        emit filterChanged();
    }
}

void FilterDiagramListModel::setModel(DiagramListModel *model)
{
    if (sourceModel() != model) {
        setSourceModel(model);
        emit modelChanged();
    }
}

DiagramTemplate *FilterDiagramListModel::diagramTemplate(int proxy) const
{
    const auto proxyIndex = index(proxy, 0);
    const auto srcIndex = mapToSource(proxyIndex);

    DiagramListModel *srcModel = static_cast<DiagramListModel *>(sourceModel());

    return srcModel->diagramTemplate(srcIndex.row());
}
bool FilterDiagramListModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    //  If no filtering, return everything
    if (_filter == Filter::None)
        return true;

    //  Filter on diagram type
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    return sourceModel()->data(index, DiagramListModel::DiagramType) == _filterString;
}

/*bool FilterDiagramListModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);
}*/
