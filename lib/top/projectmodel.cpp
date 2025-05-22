#include "projectmodel.hpp"
#include <QFileDialog>
#include <QStringLiteral>
#include <qstringliteral.h>

int ProjectModel::rowCount(const QModelIndex &parent) const { return _projects.size(); }

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0) return {};

  switch (role) {
  case static_cast<int>(Roles::ProjectPtrRole): return QVariant::fromValue(&*_projects[index.row()].impl);
  case static_cast<int>(Roles::NameRole): return _projects[index.row()].name;
  case static_cast<int>(Roles::DescriptionRole): return describe(index.row());
  case static_cast<int>(Roles::DirtyRole): return _projects[index.row()].isDirty;
  case static_cast<int>(Roles::PathRole): return _projects[index.row()].path;
  default: return {};
  }
  return {};
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0) return {};
  switch (role) {
  case static_cast<int>(Roles::NameRole): _projects[index.row()].name = value.toString(); break;
  case static_cast<int>(Roles::DirtyRole): _projects[index.row()].isDirty = value.toBool(); break;
  default: return false;
  }

  emit dataChanged(index, index);
  return true;
}

#ifdef __EMSCRIPTEN__
const auto fmt = QStringLiteral("Project %1");
#else
const auto fmt = QStringLiteral("Unnamed %1");
#endif

Pep_ISA *ProjectModel::pep10ISA() {
  static const project::Environment env{.arch = pepp::Architecture::PEP10, .level = pepp::Abstraction::ISA3};
  auto ptr = std::make_unique<Pep_ISA>(env, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size() + 1)});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep_ISA *ProjectModel::pep9ISA() {
  static const project::Environment env{.arch = pepp::Architecture::PEP9, .level = pepp::Abstraction::ISA3};
  auto ptr = std::make_unique<Pep_ISA>(env, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size() + 1)});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep_ASMB *ProjectModel::pep10ASMB(pepp::Abstraction abstraction) {
  project::Environment env{.arch = pepp::Architecture::PEP10, .level = abstraction};
  auto ptr = std::make_unique<Pep_ASMB>(env, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size() + 1)});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep_ASMB *ProjectModel::pep9ASMB() {
  project::Environment env{.arch = pepp::Architecture::PEP9, .level = pepp::Abstraction::ASMB5};
  auto ptr = std::make_unique<Pep_ASMB>(env, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back({.impl = std::move(ptr), .name = fmt.arg(_projects.size() + 1)});
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

bool ProjectModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (row < 0 || row + count > _projects.size() || count <= 0) return false;
  // row+count is one past the last element to be removed.
  beginRemoveRows(QModelIndex(), row, row + count - 1);
  // Take all pointers from project, and delete them at some point in the future rather than now.
  // This avoids a CTD where the project's data has already been deleted but that data is still bound to something in
  // QML.
  for (int i = 0; i < count; ++i) {
    auto *ptr = _projects[row].impl.release();
    ptr->setParent(this);
    // Will be deleted after re-entering event loop, which will be after the model is no longer in use.
    ptr->deleteLater();
  }
  _projects.erase(_projects.begin() + row, _projects.begin() + row + count);
  endRemoveRows();
  emit rowCountChanged(_projects.size());
  return true;
}

bool ProjectModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                            const QModelIndex &destinationParent, int destinationChild) {
  return false;
}

QHash<int, QByteArray> ProjectModel::roleNames() const {
  auto ret = QAbstractListModel::roleNames();
  ret[static_cast<int>(Roles::ProjectPtrRole)] = "project";
  ret[static_cast<int>(Roles::NameRole)] = "name";
  ret[static_cast<int>(Roles::DescriptionRole)] = "description";
  ret[static_cast<int>(Roles::DirtyRole)] = "isDirty";
  ret[static_cast<int>(Roles::PathRole)] = "path";
  return ret;
}

QString ProjectModel::describe(int index) const {
  using enum pepp::Architecture;
  if (index < 0 || index >= _projects.size()) return {};
  QMap<pepp::Architecture, QString> arch_map = {
      {PEP10, "Pep/10"},
      {PEP9, "Pep/9"},
      {PEP8, "Pep/8"},
      {RISCV, "RISC-V"},
  };
  auto abs_enum = QMetaEnum::fromType<pepp::Abstraction>();
  pepp::Architecture arch;
  pepp::Abstraction abs;
  if (auto isa = dynamic_cast<Pep_ISA *>(_projects[index].impl.get())) {
    arch = isa->architecture();
    abs = isa->abstraction();
  } else if (auto asmb = dynamic_cast<Pep_ASMB *>(_projects[index].impl.get())) {
    arch = asmb->architecture();
    abs = asmb->abstraction();
  } else return "";
  QString abs_str = abs_enum.valueToKey((int)abs);
  return QStringLiteral("%1, %2").arg(arch_map[arch], abs_str);
}

int ProjectModel::rowOf(const QObject *item) const {
  for (int i = 0; i < _projects.size(); ++i) {
    if (&*_projects[i].impl == item) return i;
  }
  return -1;
}
const std::map<std::pair<pepp::Abstraction, pepp::Architecture>, const char *> extensions = {
    {{pepp::Abstraction::ISA3, pepp::Architecture::PEP10}, "Pep/10 Object Code (*.pepo)"},
    {{pepp::Abstraction::ASMB3, pepp::Architecture::PEP10}, "Pep/10 Assembly Code (*.pep)"},
    {{pepp::Abstraction::ASMB5, pepp::Architecture::PEP10}, "Pep/10 Assembly Code (*.pep)"},
    {{pepp::Abstraction::ISA3, pepp::Architecture::PEP9}, "Pep/9 Object Code (*.pepo)"},
    {{pepp::Abstraction::ASMB5, pepp::Architecture::PEP9}, "Pep/9 Assembly Code (*.pep)"},
};

std::pair<pepp::Abstraction, pepp::Architecture> envFromPtr(const QObject *item) {
  if (auto asmb = qobject_cast<const Pep_ASMB *>(item)) {
    return {asmb->abstraction(), asmb->architecture()};
  } else if (auto isa = qobject_cast<const Pep_ISA *>(item)) {
    return {isa->abstraction(), isa->architecture()};
  }
  return {pepp::Abstraction::NO_ABS, pepp::Architecture::NO_ARCH};
}
QByteArray primaryTextFromPtr(const QObject *item) {
  if (auto asmb = qobject_cast<const Pep_ASMB *>(item)) {
    return asmb->userAsmText().toUtf8();
  } else if (auto isa = qobject_cast<const Pep_ISA *>(item)) {
    return isa->objectCodeText().toUtf8();
  }
  return "";
}
const char *recentProjectsDirKey = "recentProjectsDir";
void ProjectModel::onSave(int row) {
  if (row < 0 || row >= _projects.size()) return;

  using enum QStandardPaths::StandardLocation;
  auto ptr = _projects[row].impl.get();
  auto env = envFromPtr(ptr);
  if (env.first == pepp::Abstraction::NO_ABS) {
    qDebug() << "Unrecognized abstraction";
    return;
  }
  auto contents = primaryTextFromPtr(ptr);
#ifdef __EMSCRIPTEN__
  QString fname = "user.o";
  switch (env.first) {
  case pepp::Abstraction::ISA3: fname = "user.pepo"; break;
  case pepp::Abstraction::ASMB3: [[fallthrough]];
  case pepp::Abstraction::ASMB5: fname = "user.pep"; break;
  default: qDebug() << "No default for abstraction"; return;
  }
  QFileDialog::saveFileContent(contents, fname);
#else
  if (_projects[row].path.isEmpty()) {
    // Determine appropriate filter for project.
    auto filterIter = extensions.find(env);
    // Get last directory into which we stored files
    QSettings settings;
    settings.beginGroup("projects");
    auto d = settings.value(recentProjectsDirKey, QStandardPaths::writableLocation(DocumentsLocation)).toString();
    // Path may be empty if it is canceled, in which case we need to return early.
    _projects[row].path = QFileDialog::getSaveFileName(
        nullptr, "Save", d, filterIter != extensions.cend() ? filterIter->second : "IDK (*.pep)");
    if (_projects[row].path.isEmpty()) return;
    // Update the name field to reflect the underlying file name.
    _projects[row].name = QFileInfo(_projects[row].path).fileName();
    settings.setValue(recentProjectsDirKey, QFileInfo(_projects[row].path).absolutePath());
  }
  QFile file(_projects[row].path);
  if (!file.open(QIODevice::WriteOnly)) return;
  file.write(contents);
  file.close();
#endif

  auto index = createIndex(row, 0);
  setData(index, false, static_cast<int>(Roles::DirtyRole));
}

void init_pep10(QList<ProjectType> &vec) {
  auto a = pepp::Architecture::PEP10;
  using pepp::Abstraction;
  vec.append({.name = "Pep/10\nLevel ISA3\nBare Metal",
              .description = "Develop and debug machine language programs in bare metal mode.",
              .arch = a,
              .level = Abstraction::ISA3,
              .state = CompletionState::COMPLETE,
              .edition = 6});
  vec.append({.name = "Pep/10\nLevel Asmb3\nBare Metal",
              .description = "Develop and debug assembly language programs in bare metal mode.",
              .arch = a,
              .level = Abstraction::ASMB3,
              .state = CompletionState::COMPLETE,
              .edition = 6});
  vec.append({.name = "Pep/10\nLevel Asmb5\nFull OS",
              .description = "Develop and debug assembly language programs alongside Pep/10's operating system.\nThis "
                             "level enables you to utilize OS features using system calls for enhanced functionality.",
              .arch = a,
              .level = Abstraction::ASMB5,
              .state = CompletionState::COMPLETE,
              .edition = 6});
  vec.append(
      {.name = "Pep/10\nOS4\n", .arch = a, .level = Abstraction::OS4, .state = CompletionState::PARTIAL, .edition = 6});
  vec.append({.name = "Pep/10\nLevel Mc2\n1-Byte Bus",
              .arch = a,
              .level = Abstraction::MC2,
              .state = CompletionState::INCOMPLETE,
              .edition = 6});
  vec.append({.name = "Pep/10\nLevel Mc2\n2-Byte Bus",
              .arch = a,
              .level = Abstraction::MC2,
              .state = CompletionState::INCOMPLETE,
              .edition = 6});
}
void init_pep9(QList<ProjectType> &vec) {
  auto a = pepp::Architecture::PEP9;
  using pepp::Abstraction;
  vec.append({.name = "Pep/9\nLevel ISA3\n",
              .description = "Develop and debug machine language programs.",
              .arch = a,
              .level = Abstraction::ISA3,
              .state = CompletionState::COMPLETE,
              .edition = 5});
  vec.append(
      {.arch = a, .level = Abstraction::ASMB3, .state = CompletionState::COMPLETE, .edition = 5, .placeholder = true});
  vec.append({.name = "Pep/9\nLevel Asmb5\n",
              .description =
                  "Develop and debug assembly language programs alongside Pep/9's operating system.\nThis level "
                  "enables you to utilize OS features using trap instructions for enhanced functionality.",
              .arch = a,
              .level = Abstraction::ASMB5,
              .state = CompletionState::COMPLETE,
              .edition = 5});
  vec.append({.name = "Pep/9\nLevel OS4\n",
              .arch = a,
              .level = Abstraction::OS4,
              .state = CompletionState::INCOMPLETE,
              .edition = 5});
  vec.append({.name = "Pep/9\nLevel Mc2\n1-Byte Bus",
              .arch = a,
              .level = Abstraction::MC2,
              .state = CompletionState::INCOMPLETE,
              .edition = 5});
  vec.append({.name = "Pep/9\nLevel Mc2\n2-Byte Bus",
              .arch = a,
              .level = Abstraction::MC2,
              .state = CompletionState::INCOMPLETE,
              .edition = 5});
}
void init_pep8(QList<ProjectType> &vec) {
  auto a = pepp::Architecture::PEP8;
  using pepp::Abstraction;
  vec.append({.name = "Pep/8\nLevel ISA3\n",
              .description = "Develop and debug machine language programs.",
              .arch = a,
              .level = Abstraction::ISA3,
              .state = CompletionState::INCOMPLETE,
              .edition = 4});
  vec.append(
      {.arch = a, .level = Abstraction::ISA3, .state = CompletionState::INCOMPLETE, .edition = 4, .placeholder = true});
  vec.append({.name = "Pep/8\nLevel Asmb5\n",
              .description =
                  "Develop and debug assembly language programs alongside Pep/8's operating system.\nThis level "
                  "enables you to utilize OS features using trap instructions for enhanced functionality.",
              .arch = a,
              .level = Abstraction::ASMB5,
              .state = CompletionState::INCOMPLETE,
              .edition = 4});
  vec.append({.name = "Pep/8\nLevel OS4\n",
              .arch = a,
              .level = Abstraction::OS4,
              .state = CompletionState::INCOMPLETE,
              .edition = 4});
  vec.append({.name = "Pep/8\nLevel Mc2\n 1-Byte Bus",
              .arch = a,
              .level = Abstraction::MC2,
              .state = CompletionState::INCOMPLETE,
              .edition = 4});
}
void init_riscv(QList<ProjectType> &vec) {
  auto a = pepp::Architecture::RISCV;
  using pepp::Abstraction;
  vec.append({.arch = a,
              .level = Abstraction::ASMB3,
              .state = CompletionState::INCOMPLETE,
              .edition = 6,
              .placeholder = true});
  vec.append({.name = "RISC-V\nLevel Asmb3\nBare Metal",
              .description = "Develop and debug machine language programs in bare metal mode.",
              .arch = a,
              .level = Abstraction::ASMB3,
              .state = CompletionState::INCOMPLETE,
              .edition = 6});
}

ProjectTypeModel::ProjectTypeModel(QObject *parent) : QAbstractTableModel(parent) {
  init_pep10(_projects);
  init_pep9(_projects);
  init_pep8(_projects);
  init_riscv(_projects);
}

int ProjectTypeModel::rowCount(const QModelIndex &parent) const { return _projects.size(); }

int ProjectTypeModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant ProjectTypeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() > 1) return {};
  switch (role) {
  case static_cast<int>(Roles::NameRole): return _projects[index.row()].name;
  case static_cast<int>(Roles::DescriptionRole): return _projects[index.row()].description;
  case static_cast<int>(Roles::ArchitectureRole): return static_cast<int>(_projects[index.row()].arch);
  case static_cast<int>(Roles::EditionRole): return static_cast<int>(_projects[index.row()].edition);
  case static_cast<int>(Roles::LevelRole): return static_cast<int>(_projects[index.row()].level);
  case static_cast<int>(Roles::CompleteRole): return _projects[index.row()].state == CompletionState::COMPLETE;
  case static_cast<int>(Roles::PartiallyCompleteRole): return _projects[index.row()].state == CompletionState::PARTIAL;
  case static_cast<int>(Roles::PlaceholderRole): return static_cast<int>(_projects[index.row()].placeholder);
  default: return {};
  }
}

QHash<int, QByteArray> ProjectTypeModel::roleNames() const {
  static const QHash<int, QByteArray> ret{
      {(int)ProjectTypeModel::Roles::NameRole, "text"},
      {(int)ProjectTypeModel::Roles::DescriptionRole, "description"},
      {(int)ProjectTypeModel::Roles::ArchitectureRole, "architecture"},
      {(int)ProjectTypeModel::Roles::EditionRole, "edition"},
      {(int)ProjectTypeModel::Roles::LevelRole, "abstraction"},
      {(int)ProjectTypeModel::Roles::CompleteRole, "complete"},
      {(int)ProjectTypeModel::Roles::PartiallyCompleteRole, "partiallyComplete"},
      {(int)ProjectTypeModel::Roles::ColumnTypeRole, "columnType"},
      {(int)ProjectTypeModel::Roles::PlaceholderRole, "placeholder"},
  };
  return ret;
}

ProjectTypeFilterModel::ProjectTypeFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void ProjectTypeFilterModel::setArchitecture(pepp::Architecture arch) {
  if (_architecture == arch) return;
  _architecture = arch;
  _edition = 0;
  emit architectureChanged();
  invalidateFilter();
}

void ProjectTypeFilterModel::setEdition(int edition) {
  if (_edition == edition) return;
  _architecture = pepp::Architecture::NO_ARCH;
  _edition = edition;
  emit editionChanged();
  invalidateFilter();
}

void ProjectTypeFilterModel::setShowIncomplete(bool value) {
  if (_showIncomplete == value) return;
  _showIncomplete = value;
  emit showIncompleteChanged();
  invalidateFilter();
}

void ProjectTypeFilterModel::setShowPartial(bool value) {
  if (_showPartial == value) return;
  _showPartial = value;
  emit showPartialChanged();
  invalidateFilter();
}

bool ProjectTypeFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  auto index = sourceModel()->index(source_row, 0, source_parent);
  bool isIncomplete = !sourceModel()->data(index, static_cast<int>(ProjectTypeModel::Roles::CompleteRole)).toBool();
  bool isPartial =
      sourceModel()->data(index, static_cast<int>(ProjectTypeModel::Roles::PartiallyCompleteRole)).toBool();
  auto arch = static_cast<pepp::Architecture>(
      sourceModel()->data(index, static_cast<int>(ProjectTypeModel::Roles::ArchitectureRole)).toInt());
  auto edition = sourceModel()->data(index, static_cast<int>(ProjectTypeModel::Roles::EditionRole)).toInt();
  if (_architecture != pepp::Architecture::NO_ARCH && arch != _architecture) return false;
  else if (_edition != 0 && edition != _edition) return false;
  else if (!_showIncomplete && isIncomplete) return false;
  else if (!_showPartial && isPartial) return false;
  return true;
}
