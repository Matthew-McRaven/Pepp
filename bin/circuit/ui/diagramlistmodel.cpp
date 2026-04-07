#include "diagramlistmodel.hpp"
#include <QImage>
#include <QPixmap>
#include "schematic/blueprintlibrary.hpp"

DiagramListModel::DiagramListModel(QObject *parent) : QAbstractListModel(parent) {}

int DiagramListModel::rowCount(const QModelIndex &parent) const {
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid() || _project == nullptr) return 0;
  const auto library = _project->library();
  return library->groups().size();
}

QVariant DiagramListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || _project == nullptr) return {};
  const auto library = _project->library();
  const auto &groups = library->groups();
  const auto &group = groups.container.at(index.row());

  switch (role) {
  case Role::Name: return QString::fromStdString(group.second->name);
  case Role::Path: {
    const auto img = group.second->image;
    const std::string fname = **_project->find_file(img);
    // icon.source expects the string to start with qrc:/
    return "qrc" + QString::fromStdString(fname);
  }
  }

  //  property not found
  return {};
}

QHash<int, QByteArray> DiagramListModel::roleNames() const { return {{Role::Name, "name"}, {Role::Path, "path"}}; }

void DiagramListModel::setProject(CircuitProject *project) {
  if (_project != project) {
    _project = project;
    emit projectChanged();
    beginResetModel();
    endResetModel();
  }
}

u32 DiagramListModel::blueprint(int index) const {
  if (_project == nullptr) return schematic::BlueprintID{}.value;
  const auto library = _project->library();
  const auto &groups = library->groups();
  if (index < 0 || index >= static_cast<int>(groups.size())) return schematic::BlueprintID{}.value;
  const auto &group = groups.container.at(index);
  if (group.second->members.empty()) return schematic::BlueprintID{}.value;
  else return group.second->members.begin()->value;
}

FilterDiagramListModel::FilterDiagramListModel(QObject *parent) : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  setSortRole(Qt::DisplayRole);
}

void FilterDiagramListModel::setFilterGroupFilter(Filter filter) {
  if (_filter != filter) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
#endif
    _filter = filter;

    switch (_filter) {
    case Arrow: _filterString = "Arrow"; break;
    case Diagram: _filterString = "Diagram"; break;
    case Line: _filterString = "Line"; break;
    default: _filterString.clear();
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    invalidateFilter();
#endif
    emit filterChanged();
  }
}

void FilterDiagramListModel::setModel(DiagramListModel *model) {
  if (sourceModel() != model) {
    setSourceModel(model);
    emit modelChanged();
  }
}

bool FilterDiagramListModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
  //  If no filtering, return everything
  if (_filter == Filter::None) return true;

  //  Filter on diagram type
  QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
  return true;
  // return sourceModel()->data(index, DiagramListModel::DiagramType) == _filterString;
}

/*bool FilterDiagramListModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);
}*/
