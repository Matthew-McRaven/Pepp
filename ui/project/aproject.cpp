#include "aproject.hpp"

#include <QQmlEngine>

Pep10_ISA::Pep10_ISA(QVariant delegate, QObject *parent)
    : QObject(parent), _delegate(delegate), _memory(new ArrayRawMemory(0x10000, this)),
      _registers(new RegisterModel(this)), _flags(new FlagModel(this)), _mnemonics(new Pep10OpcodeModel(this)) {
  QQmlEngine::setObjectOwnership(_memory, QQmlEngine::CppOwnership);
  QQmlEngine::setObjectOwnership(_registers, QQmlEngine::CppOwnership);
  using RF = QSharedPointer<RegisterFormatter>;
  using TF = QSharedPointer<TextFormatter>;
  using MF = QSharedPointer<MnemonicFormatter>;
  using HF = QSharedPointer<HexFormatter>;
  using SF = QSharedPointer<SignedDecFormatter>;
  using UF = QSharedPointer<UnsignedDecFormatter>;
  using BF = QSharedPointer<BinaryFormatter>;
  auto A = []() { return 0; };
  auto X = []() { return 0; };
  auto SP = []() { return 0; };
  auto PC = []() { return 0; };
  auto IS = []() { return 1; };
  auto IS_TEXT = [&]() {
    int opcode = IS();
    auto row = _mnemonics->indexFromOpcode(opcode);
    return _mnemonics->data(_mnemonics->index(row), Qt::DisplayRole).toString();
  };
  auto OS = []() { return 0; };
  _registers->appendFormatters({TF::create("Accumulator"), HF::create(A, 2), SF::create(A, 2)});
  _registers->appendFormatters({TF::create("Index Register"), HF::create(X, 2), SF::create(X, 2)});
  _registers->appendFormatters({TF::create("Stack Pointer"), HF::create(SP, 2), UF::create(SP, 2)});
  _registers->appendFormatters({TF::create("Program Counter"), HF::create(PC, 2), UF::create(PC, 2)});
  _registers->appendFormatters({TF::create("Instruction Specifier"), BF::create(IS, 1), MF::create(IS_TEXT)});
  _registers->appendFormatters({TF::create("Operand Specifier"), HF::create(OS, 2), SF::create(OS, 2)});
  _registers->appendFormatters({TF::create("(Operand)"), TF::create("??"), TF::create("??")});

  using F = QSharedPointer<Flag>;
  auto N = []() { return false; };
  auto Z = []() { return false; };
  auto V = []() { return false; };
  auto C = []() { return false; };
  _flags->appendFlag({F::create("N", N)});
  _flags->appendFlag({F::create("Z", Z)});
  _flags->appendFlag({F::create("V", V)});
  _flags->appendFlag({F::create("C", C)});
}

project::Environment Pep10_ISA::env() const {
  return {.arch = utils::Architecture::PEP10, .level = utils::Abstraction::ISA3, .features = project::Features::None};
}

utils::Architecture Pep10_ISA::architecture() const { return utils::Architecture::PEP10; }

utils::Abstraction Pep10_ISA::abstraction() const { return utils::Abstraction::ISA3; }

ARawMemory *Pep10_ISA::memory() const { return _memory; }

QString Pep10_ISA::objectCodeText() const { return _objectCodeText; }

void Pep10_ISA::setObjectCodeText(const QString &objectCodeText) {
  if (_objectCodeText == objectCodeText)
    return;
  _objectCodeText = objectCodeText;
  emit objectCodeTextChanged();
}

void Pep10_ISA::set(int abstraction, QString value) {
  if (abstraction == static_cast<int>(utils::Abstraction::ISA3)) {
    setObjectCodeText(value);
  }
}

int ProjectModel::rowCount(const QModelIndex &parent) const { return _projects.size(); }

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0)
    return {};

  switch (role) {
  case static_cast<int>(Roles::ProjectRole):
    return QVariant::fromValue(_projects[index.row()]);
  default:
    return {};
  }
  return {};
}

Pep10_ISA *ProjectModel::pep10ISA(QVariant delegate) {
  auto ret = new Pep10_ISA(delegate);
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back(ret);
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

bool ProjectModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (row < 0 || row + count > _projects.size() || count <= 0)
    return false;
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

uint64_t mask(uint8_t byteCount) {
  if (byteCount >= 8)
    return -1;
  return (1ULL << (byteCount * 8ULL)) - 1ULL;
}
