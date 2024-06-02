#include "aproject.hpp"
#include <QQmlEngine>
#include <sstream>
#include "bits/strings.hpp"
#include "help/builtins/figure.hpp"
#include "helpers/asmb.hpp"
#include "isa/pep10.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include "targets/pep10/isa3/system.hpp"

struct Pep10OpcodeInit {
  explicit Pep10OpcodeInit(OpcodeModel *model) {
    static const auto mnemonicEnum = QMetaEnum::fromType<isa::Pep10::Mnemonic>();
    static const auto addressEnum = QMetaEnum::fromType<isa::Pep10::AddressingMode>();
    for (int it = 0; it < 256; it++) {
      auto op = isa::Pep10::opcodeLUT[it];
      if (!op.valid)
        continue;
      QString formatted;
      // instr.unary indicates if the instruction is hardware-unary (i.e., it could be a nonunary trap SCALL).
      // This is why we test the addressing mode instead, since nonunary traps will have an addressing mode.
      if (op.mode == isa::Pep10::AddressingMode::NONE) {
        formatted = QString(mnemonicEnum.valueToKey((int)op.instr.mnemon)).toUpper();
      } else {
        formatted = u"%1, %2"_qs.arg(QString(mnemonicEnum.valueToKey((int)op.instr.mnemon)).toUpper(),
                                     QString(addressEnum.valueToKey((int)op.mode)).toLower());
      }
      model->appendRow(formatted, it);
    }
  }
};

// Prevent WASM-ld error due to multiply defined symbol in static lib.
namespace {
auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};
}

Pep10_ISA::Pep10_ISA(QVariant delegate, QObject *parent)
    : QObject(parent), _delegate(delegate), _memory(new ArrayRawMemory(0x10000, this)),
      _registers(new RegisterModel(this)), _flags(new FlagModel(this)) {
  QQmlEngine::setObjectOwnership(_memory, QQmlEngine::CppOwnership);
  QQmlEngine::setObjectOwnership(_registers, QQmlEngine::CppOwnership);
  _system.clear();
  assert(_system.isNull());
  using RF = QSharedPointer<RegisterFormatter>;
  using TF = QSharedPointer<TextFormatter>;
  using MF = QSharedPointer<MnemonicFormatter>;
  using HF = QSharedPointer<HexFormatter>;
  using SF = QSharedPointer<SignedDecFormatter>;
  using UF = QSharedPointer<UnsignedDecFormatter>;
  using BF = QSharedPointer<BinaryFormatter>;
  using OF = QSharedPointer<OptionalFormatter>;
  auto _register = [](isa::Pep10::Register r, auto *system) {
    if (system == nullptr)
      return quint16{0};
    quint16 ret = 0;
    auto cpu = system->cpu();
    targets::pep10::isa::readRegister(cpu->regs(), r, ret, gs);
    return ret;
  };
  // BUG: _system seems to be getting deleted very easily. It probably shouldn't be a shared pointer.
  // DO NOT CAPTURE _system INDIRECTLY VIA ANOTHER LAMBDA. It will crash.
  auto A = [&_register, this]() { return _register(isa::Pep10::Register::A, &*this->_system); };
  auto X = [&_register, this]() { return _register(isa::Pep10::Register::X, &*this->_system); };
  auto SP = [&_register, this]() { return _register(isa::Pep10::Register::SP, &*this->_system); };
  auto PC = [&_register, this]() { return _register(isa::Pep10::Register::PC, &*this->_system); };
  auto IS = [&_register, this]() { return _register(isa::Pep10::Register::IS, &*this->_system); };
  auto IS_TEXT = [&_register, this]() {
    int opcode = _register(isa::Pep10::Register::IS, &*this->_system);
    auto row = mnemonics()->indexFromOpcode(opcode);
    return mnemonics()->data(mnemonics()->index(row), Qt::DisplayRole).toString();
  };
  auto OS = [&_register, this]() { return _register(isa::Pep10::Register::OS, &*this->_system); };
  auto notU = [&_register, this]() {
    auto is = _register(isa::Pep10::Register::IS, &*this->_system);
    auto op = isa::Pep10::opcodeLUT[is];
    return !op.instr.unary;
  };
  _registers->appendFormatters({TF::create("Accumulator"), HF::create(A, 2), SF::create(A, 2)});
  _registers->appendFormatters({TF::create("Index Register"), HF::create(X, 2), SF::create(X, 2)});
  _registers->appendFormatters({TF::create("Stack Pointer"), HF::create(SP, 2), UF::create(SP, 2)});
  _registers->appendFormatters({TF::create("Program Counter"), HF::create(PC, 2), UF::create(PC, 2)});
  _registers->appendFormatters({TF::create("Instruction Specifier"), BF::create(IS, 1), MF::create(IS_TEXT)});
  _registers->appendFormatters(
      {TF::create("Operand Specifier"), OF::create(HF::create(OS, 2), notU), OF::create(SF::create(OS, 2), notU)});
  _registers->appendFormatters(
      {TF::create("(Operand)"), OF::create(TF::create("??"), notU), OF::create(TF::create("??"), notU)});
  connect(this, &Pep10_ISA::beginResetModel, _registers, &RegisterModel::onBeginExternalReset);
  connect(this, &Pep10_ISA::endResetModel, _registers, &RegisterModel::onEndExternalReset);

  using F = QSharedPointer<Flag>;
  auto _flag = [](isa::Pep10::CSR s, auto *system) {
    if (system == nullptr)
      return false;
    bool ret = 0;
    auto cpu = system->cpu();
    targets::pep10::isa::readCSR(cpu->csrs(), s, ret, gs);
    return ret;
  };
  // See above for wanings on _system pointer.
  auto N = [&_flag, this]() { return _flag(isa::Pep10::CSR::N, &*this->_system); };
  auto Z = [&_flag, this]() { return _flag(isa::Pep10::CSR::Z, &*this->_system); };
  auto V = [&_flag, this]() { return _flag(isa::Pep10::CSR::V, &*this->_system); };
  auto C = [&_flag, this]() { return _flag(isa::Pep10::CSR::C, &*this->_system); };
  _flags->appendFlag({F::create("N", N)});
  _flags->appendFlag({F::create("Z", Z)});
  _flags->appendFlag({F::create("V", V)});
  _flags->appendFlag({F::create("C", C)});
  connect(this, &Pep10_ISA::beginResetModel, _flags, &FlagModel::onBeginExternalReset);
  connect(this, &Pep10_ISA::endResetModel, _flags, &FlagModel::onEndExternalReset);
}

project::Environment Pep10_ISA::env() const {
  return {.arch = utils::Architecture::PEP10, .level = utils::Abstraction::ISA3, .features = project::Features::None};
}

utils::Architecture Pep10_ISA::architecture() const { return utils::Architecture::PEP10; }

utils::Abstraction Pep10_ISA::abstraction() const { return utils::Abstraction::ISA3; }

ARawMemory *Pep10_ISA::memory() const { return _memory; }

OpcodeModel *Pep10_ISA::mnemonics() const {
  static OpcodeModel *model = new OpcodeModel();
  static Pep10OpcodeInit ret(model);
  return model;
}

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

int Pep10_ISA::allowedDebugging() const { return -1; }

int Pep10_ISA::allowedSteps() const { return -1; }

bool Pep10_ISA::onSaveCurrent() { return false; }

bool Pep10_ISA::onLoadObject() {

  emit beginResetModel();
  auto book = helpers::book(6);
  auto os = book->findFigure("os", "pep10baremetal");
  auto osContents = os->typesafeElements()["pep"]->contents;
  auto macroRegistry = helpers::registry(book, {});
  helpers::AsmHelper helper(macroRegistry, osContents);
  auto result = helper.assemble();
  if (!result) {
    qWarning() << "Assembly failed";
    return false;
  }
  std::string objText;
  auto bytes = bits::asciiHexToByte({objText.data(), objText.size()});
  // Need to reload to properly compute segment addresses. Store in temp
  // directory to prevent clobbering local file contents.
  auto elf = helper.elf(*bytes);
  {
    std::stringstream s;
    elf->save(s);
    s.seekg(0, std::ios::beg);
    elf->load(s);
  }
  _system = targets::pep10::isa::systemFromElf(*elf, true);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::A, 0x11, gs);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::X, 9, gs);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::SP, 11, gs);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::OS, 7, gs);
  emit endResetModel();
  return true;
}

bool Pep10_ISA::onExecute() { return false; }

bool Pep10_ISA::onDebuggingStart() { return false; }

bool Pep10_ISA::onDebuggingContinue() { return false; }

bool Pep10_ISA::onDebuggingPause() { return false; }

bool Pep10_ISA::onDebuggingStop() { return false; }

bool Pep10_ISA::onISARemoveAllBreakpoints() { return false; }

bool Pep10_ISA::onISAStep() { return false; }

bool Pep10_ISA::onISAStepOver() { return false; }

bool Pep10_ISA::onISAStepInto() { return false; }

bool Pep10_ISA::onISAStepOut() { return false; }

bool Pep10_ISA::onClearCPU() { return false; }

bool Pep10_ISA::onClearMemory() { return false; }

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

project::DebugEnableFlags::DebugEnableFlags(QObject *parent) : QObject(parent) {}

project::StepEnableFlags::StepEnableFlags(QObject *parent) : QObject(parent) {}
