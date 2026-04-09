#include "blueprintlibrarymodel.hpp"
#include <QImage>
#include <QPixmap>
#include "schematic/blueprintlibrary.hpp"

BlueprintLibraryModel::BlueprintLibraryModel(QObject *parent) : QAbstractListModel(parent) {}

int BlueprintLibraryModel::rowCount(const QModelIndex &parent) const {
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid() || _project == nullptr) return 0;
  const auto library = _project->library();
  return library->groups().size();
}

QVariant BlueprintLibraryModel::data(const QModelIndex &index, int role) const {
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
  case Role::Id: return group.first.value;
  }

  //  property not found
  return {};
}

QHash<int, QByteArray> BlueprintLibraryModel::roleNames() const {
  return {{Role::Name, "name"}, {Role::Path, "path"}, {Role::Id, "id"}};
}

void BlueprintLibraryModel::setProject(CircuitProject *project) {
  if (_project != project) {
    beginResetModel();
    _project = project;
    endResetModel();
    emit projectChanged();
  }
}

u32 BlueprintLibraryModel::blueprint(int index) const {
  if (_project == nullptr) return schematic::BlueprintID{}.value;
  const auto library = _project->library();
  const auto &groups = library->groups();
  if (index < 0 || index >= static_cast<int>(groups.size())) return schematic::BlueprintID{}.value;
  const auto &group = groups.container.at(index);
  if (group.second->members.empty()) return schematic::BlueprintID{}.value;
  else return group.second->members.begin()->value;
}