#include "projectmodel.hpp"

int ProjectModel::rowCount(const QModelIndex &parent) const { return _projects.size(); }

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0) return {};

  switch (role) {
  case static_cast<int>(Roles::ProjectRole): return QVariant::fromValue(&*_projects[index.row()]);
  default: return {};
  }
  return {};
}

Pep_ISA *ProjectModel::pep10ISA(QVariant delegate) {
  static const project::Environment env{.arch = builtins::Architecture::PEP10, .level = builtins::Abstraction::ISA3};
  auto ptr = std::make_unique<Pep_ISA>(env, delegate, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back(std::move(ptr));
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep_ISA *ProjectModel::pep9ISA(QVariant delegate) {
  static const project::Environment env{.arch = builtins::Architecture::PEP9, .level = builtins::Abstraction::ISA3};
  auto ptr = std::make_unique<Pep_ISA>(env, delegate, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back(std::move(ptr));
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep10_ASMB *ProjectModel::pep10ASMB(QVariant delegate, builtins::Abstraction abstraction) {
  auto ptr = std::make_unique<Pep10_ASMB>(delegate, abstraction, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back(std::move(ptr));
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

bool ProjectModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (row < 0 || row + count > _projects.size() || count <= 0) return false;
  // row+count is one past the last element to be removed.
  beginRemoveRows(QModelIndex(), row, row + count - 1);
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
  ret[static_cast<int>(Roles::ProjectRole)] = "ProjectRole";
  return ret;
}

QString ProjectModel::describe(int index) const {
  if (index < 0 || index >= _projects.size()) return {};
  QMap<builtins::Architecture, QString> arch_map = {
      {builtins::Architecture::PEP10, "Pep/10"},
      {builtins::Architecture::PEP9, "Pep/9"},
      {builtins::Architecture::PEP8, "Pep/8"},
      {builtins::Architecture::RISCV, "RISC-V"},
  };
  auto abs_enum = QMetaEnum::fromType<builtins::Abstraction>();
  builtins::Architecture arch;
  builtins::Abstraction abs;
  if (auto isa = dynamic_cast<Pep_ISA *>(_projects[index].get())) {
    arch = isa->architecture();
    abs = isa->abstraction();
  } else if (auto asmb = dynamic_cast<Pep10_ASMB *>(_projects[index].get())) {
    arch = asmb->architecture();
    abs = asmb->abstraction();
  }
  QString abs_str = abs_enum.valueToKey((int)abs);
  return QStringLiteral("%1, %2").arg(arch_map[arch], abs_str);
}
