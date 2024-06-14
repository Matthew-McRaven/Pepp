#include "aproject.hpp"
#include <QQmlEngine>
#include <sstream>
#include "bits/strings.hpp"
#include "cpu/formats.hpp"
#include "help/builtins/figure.hpp"
#include "helpers/asmb.hpp"
#include "isa/pep10.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/broadcast/pubsub.hpp"
#include "sim/device/simple_bus.hpp"
#include "sim/trace2/buffers.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include "targets/pep10/isa3/system.hpp"
#include "text/editor/object.hpp"

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
    : QObject(parent), _delegate(delegate), _tb(QSharedPointer<sim::trace2::InfiniteBuffer>::create()),
      _memory(nullptr), _registers(new RegisterModel(this)), _flags(new FlagModel(this)) {
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
  using VF = QSharedPointer<VariableByteLengthFormatter>;
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
  auto operand = [&_register, this]() {
    auto system = &*this->_system;
    return system->cpu()->currentOperand().value_or(0);
  };
  _registers->appendFormatters({TF::create("Accumulator"), HF::create(A, 2), SF::create(A, 2)});
  _registers->appendFormatters({TF::create("Index Register"), HF::create(X, 2), SF::create(X, 2)});
  _registers->appendFormatters({TF::create("Stack Pointer"), HF::create(SP, 2), UF::create(SP, 2)});
  _registers->appendFormatters({TF::create("Program Counter"), HF::create(PC, 2), UF::create(PC, 2)});
  _registers->appendFormatters({TF::create("Instruction Specifier"), BF::create(IS, 1), MF::create(IS_TEXT)});
  _registers->appendFormatters(
      {TF::create("Operand Specifier"), OF::create(HF::create(OS, 2), notU), OF::create(SF::create(OS, 2), notU)});
  auto length = [&_register, this]() {
    auto is = _register(isa::Pep10::Register::IS, &*this->_system);
    return isa::Pep10::operandBytes(is);
  };
  auto opr_hex = HF::create(operand, 2);
  auto opr_dec = SF::create(operand, 2);
  auto opr_hex_wrapped = VF::create(OF::create(opr_hex, notU), length, 2);
  auto opr_dec_wrapped = VF::create(OF::create(opr_dec, notU), length, 2);
  _registers->appendFormatters({TF::create("(Operand)"), opr_hex_wrapped, opr_dec_wrapped});
  connect(this, &Pep10_ISA::updateGUI, _registers, &RegisterModel::onUpdateGUI);

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
  connect(this, &Pep10_ISA::updateGUI, _flags, &FlagModel::onUpdateGUI);

  auto book = helpers::book(6);
  auto os = book->findFigure("os", "pep10baremetal");
  auto osContents = os->typesafeElements()["pep"]->contents;
  auto macroRegistry = helpers::registry(book, {});
  helpers::AsmHelper helper(macroRegistry, osContents);
  auto result = helper.assemble();
  if (!result) {
    qWarning() << "Default OS assembly failed";
  }
  // Need to reload to properly compute segment addresses. Store in temp
  // directory to prevent clobbering local file contents.
  _elf = helper.elf();
  {
    std::stringstream s;
    _elf->save(s);
    s.seekg(0, std::ios::beg);
    _elf->load(s);
  }
  _system = targets::pep10::isa::systemFromElf(*_elf, true);
  auto sink = QSharedPointer<sim::trace2::TranslatingModifiedAddressSink<quint16>>::create(_system->pathManager(),
                                                                                           _system->bus());
  _system->bus()->setBuffer(&*_tb);
  //_system->cpu()->setBuffer(&*_tb);
  //_system->cpu()->trace(true);
  _memory = new SimulatorRawMemory(_system->bus(), sink, this);
  connect(this, &Pep10_ISA::updateGUI, _memory, &SimulatorRawMemory::onUpdateGUI);
  QQmlEngine::setObjectOwnership(_memory, QQmlEngine::CppOwnership);
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

int Pep10_ISA::allowedDebugging() const {
  using D = project::DebugEnableFlags;
  switch (_state) {
  case State::Halted: return D::Start | D::LoadObject | D::Execute;
  case State::DebugPaused: return D::Continue | D::Stop;
  case State::NormalExec: return D::Stop;
  case State::DebugExec: return D::Stop;
  default: return 0b0;
  }
}

int Pep10_ISA::allowedSteps() const {
  if (_state != State::DebugPaused) return 0b0;
  using S = project::StepEnableFlags::Value;
  quint16 pc = 0;
  targets::pep10::isa::readRegister(_system->cpu()->regs(), isa::Pep10::Register::PC, pc, gs);
  quint8 is;
  _system->bus()->read(pc, {&is, 1}, gs);
  if (isa::Pep10::isCall(is)) return S::Step | S::StepOver | S::StepOut | S::StepInto;
  return S::Step | S::StepOver | S::StepOut;
}

QString Pep10_ISA::charIn() const { return _charIn; }

void Pep10_ISA::setCharIn(QString value) { _charIn = value; }

QString Pep10_ISA::charOut() const {
  if (auto charOut = _system->output("charOut"); charOut) {
    auto charOutEndpoint = charOut->endpoint();
    charOutEndpoint->set_to_head();
    int index = 0;
    QString out;
    for (auto next = charOutEndpoint->next_value(); next.has_value(); next = charOutEndpoint->next_value()) {
      if (out.capacity() < index + 1) out.reserve(2 * (index + 1));
      out.append(char(*next));
    }
    return out;
  }
  return u""_qs;
}

bool Pep10_ISA::onSaveCurrent() { return false; }

bool Pep10_ISA::onLoadObject() {
  static ObjectUtilities utils;
  _tb->clear();
  // Only enable trace while running the program to prevent spurious changed highlights.
  _system->bus()->trace(false);
  std::string objText = utils.format(objectCodeText(), false).toStdString();
  auto bytes = bits::asciiHexToByte({objText.data(), objText.size()});
  if (!bytes) {
    qWarning() << "Invalid object code, probably invalid hex characters.";
    return false;
  }
  auto bus = _system->bus();
  bus->clear(0);
  // Reload OS into memory.
  targets::pep10::isa::loadElfSegments(*bus, *_elf);
  // Load user program into memory.
  bus->write(0, *bytes, gs);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::A, 0x11, gs);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::X, 9, gs);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::SP, 11, gs);
  _memory->setSP(-1);
  _memory->setPC(-1, -1);
  targets::pep10::isa::writeRegister(_system->cpu()->regs(), isa::Pep10::Register::OS, 7, gs);
  emit updateGUI(_tb->cbegin());
  return true;
}

bool Pep10_ISA::onExecute() {
  prepareSim();

  auto pwrOff = _system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  bool noMMI = false;
  // Only enable trace while running the program to prevent spurious changed highlights.
  _system->bus()->trace(true);
  // Step for a small number of instructions or until we write to pwrOff.
  try {
    while (_system->currentTick() < 800 && !endpoint->next_value().has_value())
      _system->tick(sim::api2::Scheduler::Mode::Jump);
  } catch (const sim::api2::memory::Error &e) {
    if (e.type() == sim::api2::memory::Error::Type::NeedsMMI) {
      noMMI = true;
    } else
      std::cerr << "Memory error: " << e.what() << std::endl;
    // Handle illegal opcodes or program crashes.
  } catch (const std::logic_error &e) {
    std::cerr << e.what() << std::endl;
  }
  _system->bus()->trace(false);
  prepareGUIUpdate(_tb->cbegin());
  return true;
}

bool Pep10_ISA::onDebuggingStart() {
  prepareSim();
  _state = State::DebugPaused;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  _system->bus()->trace(true);
  return true;
}

bool Pep10_ISA::onDebuggingContinue() {
  _state = State::DebugExec;
  emit allowedDebuggingChanged();
  auto pwrOff = _system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  auto from = _tb->cend();
  bool err = false;
  try {
    while (_system->currentTick() < 800 && !endpoint->next_value().has_value())
      _system->tick(sim::api2::Scheduler::Mode::Jump);
  } catch (const sim::api2::memory::Error &e) {
    err = true;
    if (e.type() == sim::api2::memory::Error::Type::NeedsMMI) {
    } else
      std::cerr << "Memory error: " << e.what() << std::endl;
    // Handle illegal opcodes or program crashes.
  } catch (const std::logic_error &e) {
    err = true;
    std::cerr << e.what() << std::endl;
  }
  onDebuggingStop();
  prepareGUIUpdate(from);
  return true;
}

bool Pep10_ISA::onDebuggingPause() { return false; }

bool Pep10_ISA::onDebuggingStop() {
  _system->bus()->trace(false);
  _state = State::Halted;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  return true;
}

bool Pep10_ISA::onISARemoveAllBreakpoints() { return false; }

bool Pep10_ISA::onISAStep() {
  auto pwrOff = _system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  auto from = _tb->cend();
  bool err = false;
  try {
    _system->tick(sim::api2::Scheduler::Mode::Jump);
  } catch (const sim::api2::memory::Error &e) {
    err = true;
    if (e.type() == sim::api2::memory::Error::Type::NeedsMMI) {
    } else
      std::cerr << "Memory error: " << e.what() << std::endl;
    // Handle illegal opcodes or program crashes.
  } catch (const std::logic_error &e) {
    err = true;
    std::cerr << e.what() << std::endl;
  }
  if (endpoint->next_value().has_value()) onDebuggingStop();
  prepareGUIUpdate(from);
  return true;
}

bool Pep10_ISA::onISAStepOver() { return false; }

bool Pep10_ISA::onISAStepInto() { return false; }

bool Pep10_ISA::onISAStepOut() { return false; }

bool Pep10_ISA::onClearCPU() { return false; }

bool Pep10_ISA::onClearMemory() { return false; }

void Pep10_ISA::prepareSim() {
  // Ensure latests changes to object code pane are reflected in simulator.
  onLoadObject();
  _tb->clear();
  _system->init();
  auto pwrOff = _system->output("pwrOff");
  auto charOut = _system->output("charOut");
  charOut->clear(0);
  pwrOff->clear(0);

  auto charIn = _system->input("charIn");
  charIn->clear(0);
  auto charInEndpoint = charIn->endpoint();
  for (int it = 0; it < _charIn.size(); it++) charInEndpoint->append_value(_charIn[it].toLatin1());
}

void Pep10_ISA::prepareGUIUpdate(sim::api2::trace::FrameIterator from) {
  // Update cpu-dependent fields in memory before triggering a GUI update.
  quint8 is;
  // Use cached PC
  quint16 sp, pc = _system->cpu()->startingPC();
  targets::pep10::isa::readRegister(_system->cpu()->regs(), isa::Pep10::Register::SP, sp, gs);
  _system->bus()->read(pc, {&is, 1}, gs);
  _memory->setSP(sp);
  _memory->setPC(pc, pc + (isa::Pep10::opcodeLUT[is].instr.unary ? 0 : 2));
  emit charOutChanged();
  emit updateGUI(from);
}

int ProjectModel::rowCount(const QModelIndex &parent) const { return _projects.size(); }

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= _projects.size() || index.column() != 0) return {};

  switch (role) {
  case static_cast<int>(Roles::ProjectRole): return QVariant::fromValue(&*_projects[index.row()]);
  default: return {};
  }
  return {};
}

Pep10_ISA *ProjectModel::pep10ISA(QVariant delegate) {
  auto ptr = std::make_unique<Pep10_ISA>(delegate, nullptr);
  auto ret = &*ptr;
  QQmlEngine::setObjectOwnership(ret, QQmlEngine::CppOwnership);
  beginInsertRows(QModelIndex(), _projects.size(), _projects.size());
  _projects.push_back(std::move(ptr));
  endInsertRows();
  emit rowCountChanged(_projects.size());
  return ret;
}

Pep10_ASMB *ProjectModel::pep10ASMB(QVariant delegate) {
  auto ptr = std::make_unique<Pep10_ASMB>(delegate, nullptr);
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

project::DebugEnableFlags::DebugEnableFlags(QObject *parent) : QObject(parent) {}

project::StepEnableFlags::StepEnableFlags(QObject *parent) : QObject(parent) {}

Pep10_ASMB::Pep10_ASMB(QVariant delegate, QObject *parent) : Pep10_ISA(delegate, parent) {}

void Pep10_ASMB::set(int abstraction, QString value) {
  if (abstraction == static_cast<int>(utils::Abstraction::ASMB5)) {
    setUserAsmText(value);
  }
}

QString Pep10_ASMB::userAsmText() const { return _userAsmText; }

void Pep10_ASMB::setUserAsmText(const QString &userAsmText) {

  if (_userAsmText == userAsmText) return;
  _userAsmText = userAsmText;
  emit userAsmTextChanged();
}

project::Environment Pep10_ASMB::env() const {
  return {.arch = utils::Architecture::PEP10, .level = utils::Abstraction::ASMB5, .features = project::Features::None};
}

utils::Architecture Pep10_ASMB::architecture() const { return utils::Architecture::PEP10; }

utils::Abstraction Pep10_ASMB::abstraction() const { return utils::Abstraction::ASMB5; }

int Pep10_ASMB::allowedDebugging() const {
  using D = project::DebugEnableFlags;
  switch (_state) {
  case State::Halted: return D::Start | D::Execute;
  case State::DebugPaused: return D::Continue | D::Stop;
  case State::NormalExec: return D::Stop;
  case State::DebugExec: return D::Stop;
  default: return 0b0;
  }
}
void Pep10_ASMB::prepareSim() {}

void Pep10_ASMB::prepareGUIUpdate(sim::api2::trace::FrameIterator from) {}
