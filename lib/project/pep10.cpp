#include "./pep10.hpp"
#include <QQmlEngine>
#include <elfio/elfio.hpp>
#include <sstream>
#include "asm/pas/operations/pepp/bytes.hpp"
#include "bits/strings.hpp"
#include "cpu/formats.hpp"
#include "builtins/figure.hpp"
#include "helpers/asmb.hpp"
#include "isa/pep10.hpp"
#include "sim/api2/trace/buffer.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/broadcast/pubsub.hpp"
#include "sim/device/simple_bus.hpp"
#include "sim/trace2/buffers.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include "targets/pep10/isa3/system.hpp"
#include "text/editor/object.hpp"
#include "utils/strings.hpp"

// Prevent WASM-ld error due to multiply defined symbol in static lib.
namespace {
auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};
}

struct Pep10OpcodeInit {
  explicit Pep10OpcodeInit(OpcodeModel *model) {
    static const auto mnemonicEnum = QMetaEnum::fromType<isa::Pep10::Mnemonic>();
    static const auto addressEnum = QMetaEnum::fromType<isa::Pep10::AddressingMode>();
    for (int it = 0; it < 256; it++) {
      auto op = isa::Pep10::opcodeLUT[it];
      if (!op.valid) continue;
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

struct SystemAssembly {
  QSharedPointer<ELFIO::elfio> elf;
  QSharedPointer<targets::pep10::isa::System> system;
};

QSharedPointer<macro::Registry> cs6e_macros() {
  auto book = helpers::book(6);
  return helpers::registry(book, {});
}
SystemAssembly make_isa_system() {
  auto book = helpers::book(6);
  auto os = book->findFigure("os", "pep10baremetal");
  auto osContents = os->typesafeElements()["pep"]->contents;
  auto macroRegistry = cs6e_macros();
  helpers::AsmHelper helper(macroRegistry, osContents);
  auto result = helper.assemble();
  if (!result) {
    qWarning() << "Default OS assembly failed";
  }
  SystemAssembly ret;
  // Need to reload to properly compute segment addresses.
  ret.elf = helper.elf();
  {
    std::stringstream s;
    ret.elf->save(s);
    s.seekg(0, std::ios::beg);
    ret.elf->load(s);
  }
  ret.system = targets::pep10::isa::systemFromElf(*ret.elf, true);
  return ret;
}

QString cs6e_os() {
  auto book = helpers::book(6);
  auto os = book->findFigure("os", "pep10os");
  return os->typesafeElements()["pep"]->contents;
}
SystemAssembly make_asmb_system(QString os) {
  auto macroRegistry = cs6e_macros();
  helpers::AsmHelper helper(macroRegistry, os);
  auto result = helper.assemble();
  if (!result) {
    qWarning() << "Default OS assembly failed";
  }
  SystemAssembly ret;
  // Need to reload to properly compute segment addresses. Store in temp
  // directory to prevent clobbering local file contents.
  ret.elf = helper.elf();
  {
    std::stringstream s;
    ret.elf->save(s);
    s.seekg(0, std::ios::beg);
    ret.elf->load(s);
  }
  ret.system = targets::pep10::isa::systemFromElf(*ret.elf, true);
  return ret;
}

RegisterModel *register_model(targets::pep10::isa::System *system, OpcodeModel *opcodes, QObject *parent = nullptr) {
  using RF = QSharedPointer<RegisterFormatter>;
  using TF = QSharedPointer<TextFormatter>;
  using MF = QSharedPointer<MnemonicFormatter>;
  using HF = QSharedPointer<HexFormatter>;
  using SF = QSharedPointer<SignedDecFormatter>;
  using UF = QSharedPointer<UnsignedDecFormatter>;
  using BF = QSharedPointer<BinaryFormatter>;
  using OF = QSharedPointer<OptionalFormatter>;
  using VF = QSharedPointer<VariableByteLengthFormatter>;
  auto ret = new RegisterModel(parent);
  auto _register = [](isa::Pep10::Register r, auto *system) {
    if (system == nullptr) return quint16{0};
    quint16 ret = 0;
    auto cpu = system->cpu();
    targets::pep10::isa::readRegister(cpu->regs(), r, ret, gs);
    return ret;
  };
  // BUG: _system seems to be getting deleted very easily. It probably shouldn't be a shared pointer.
  // DO NOT CAPTURE _system INDIRECTLY VIA ANOTHER LAMBDA. It will crash.
  auto A = [=]() { return _register(isa::Pep10::Register::A, system); };
  auto X = [=]() { return _register(isa::Pep10::Register::X, system); };
  auto SP = [=]() { return _register(isa::Pep10::Register::SP, system); };
  auto PC = [=]() { return _register(isa::Pep10::Register::PC, system); };
  auto IS = [=]() { return _register(isa::Pep10::Register::IS, system); };
  auto IS_TEXT = [=]() {
    int opcode = _register(isa::Pep10::Register::IS, system);
    auto row = opcodes->indexFromOpcode(opcode);
    return opcodes->data(opcodes->index(row), Qt::DisplayRole).toString();
  };
  auto OS = [=]() { return _register(isa::Pep10::Register::OS, system); };
  auto notU = [=]() {
    auto is = _register(isa::Pep10::Register::IS, system);
    auto op = isa::Pep10::opcodeLUT[is];
    return !op.instr.unary;
  };
  auto operand = [=]() { return system->cpu()->currentOperand().value_or(0); };
  ret->appendFormatters({TF::create("Accumulator"), HF::create(A, 2), SF::create(A, 2)});
  ret->appendFormatters({TF::create("Index Register"), HF::create(X, 2), SF::create(X, 2)});
  ret->appendFormatters({TF::create("Stack Pointer"), HF::create(SP, 2), UF::create(SP, 2)});
  ret->appendFormatters({TF::create("Program Counter"), HF::create(PC, 2), UF::create(PC, 2)});
  ret->appendFormatters({TF::create("Instruction Specifier"), BF::create(IS, 1), MF::create(IS_TEXT)});
  ret->appendFormatters(
      {TF::create("Operand Specifier"), OF::create(HF::create(OS, 2), notU), OF::create(SF::create(OS, 2), notU)});
  auto length = [=]() {
    auto is = _register(isa::Pep10::Register::IS, system);
    return isa::Pep10::operandBytes(is);
  };
  auto opr_hex = HF::create(operand, 2);
  auto opr_dec = SF::create(operand, 2);
  auto opr_hex_wrapped = VF::create(OF::create(opr_hex, notU), length, 2);
  auto opr_dec_wrapped = VF::create(OF::create(opr_dec, notU), length, 2);
  ret->appendFormatters({TF::create("(Operand)"), opr_hex_wrapped, opr_dec_wrapped});
  return ret;
}

FlagModel *flag_mode(targets::pep10::isa::System *system, QObject *parent = nullptr) {
  using F = QSharedPointer<Flag>;
  auto ret = new FlagModel(parent);
  auto _flag = [](isa::Pep10::CSR s, auto *system) {
    if (system == nullptr) return false;
    bool ret = 0;
    auto cpu = system->cpu();
    targets::pep10::isa::readCSR(cpu->csrs(), s, ret, gs);
    return ret;
  };
  // See above for wanings on _system pointer.
  auto N = [=]() { return _flag(isa::Pep10::CSR::N, system); };
  auto Z = [=]() { return _flag(isa::Pep10::CSR::Z, system); };
  auto V = [=]() { return _flag(isa::Pep10::CSR::V, system); };
  auto C = [=]() { return _flag(isa::Pep10::CSR::C, system); };
  ret->appendFlag({F::create("N", N)});
  ret->appendFlag({F::create("Z", Z)});
  ret->appendFlag({F::create("V", V)});
  ret->appendFlag({F::create("C", C)});
  return ret;
}

Pep10_ISA::Pep10_ISA(QVariant delegate, QObject *parent, bool initializeSystem)
    : QObject(parent), _delegate(delegate), _tb(QSharedPointer<sim::trace2::InfiniteBuffer>::create()),
      _memory(nullptr), _registers(nullptr), _flags(nullptr) {
  _system.clear();
  assert(_system.isNull());

  if (initializeSystem) {
    auto elfsys = make_isa_system();
    _elf = elfsys.elf;
    _system = elfsys.system;
    _system->bus()->setBuffer(&*_tb);
    bindToSystem();
  }
  connect(this, &Pep10_ISA::deferredExecution, this, &Pep10_ISA::onDeferredExecution, Qt::QueuedConnection);
}

void Pep10_ISA::bindToSystem() {
  _flags = flag_mode(&*_system, this);
  // Use old-style connections to avoid a linker error in WASM.
  // For some reason, new-style connects cause LD to insert a 0-arg updateGUI into the object file.
  // We can defeat the linker with the following Qt macros.
  connect(this, SIGNAL(updateGUI(sim::api2::trace::FrameIterator)), _flags, SLOT(onUpdateGUI()));
  QQmlEngine::setObjectOwnership(_flags, QQmlEngine::CppOwnership);

  _registers = register_model(&*_system, mnemonics(), this);
  connect(this, SIGNAL(updateGUI(sim::api2::trace::FrameIterator)), _registers, SLOT(onUpdateGUI()));
  QQmlEngine::setObjectOwnership(_registers, QQmlEngine::CppOwnership);

  using TMAS = sim::trace2::TranslatingModifiedAddressSink<quint16>;
  auto sink = QSharedPointer<TMAS>::create(_system->pathManager(), _system->bus());

  _memory = new SimulatorRawMemory(_system->bus(), sink, this);
  connect(this, SIGNAL(updateGUI(sim::api2::trace::FrameIterator)), _memory,
          SLOT(onUpdateGUI(sim::api2::trace::FrameIterator)));
  QQmlEngine::setObjectOwnership(_memory, QQmlEngine::CppOwnership);

}

project::Environment Pep10_ISA::env() const {
  using namespace builtins;
  return {.arch = Architecture::PEP10, .level = Abstraction::ISA3, .features = project::Features::None};
}

builtins::Architecture Pep10_ISA::architecture() const { return builtins::Architecture::PEP10; }

builtins::Abstraction Pep10_ISA::abstraction() const { return builtins::Abstraction::ISA3; }

ARawMemory *Pep10_ISA::memory() const { return _memory; }

OpcodeModel *Pep10_ISA::mnemonics() const {
  static OpcodeModel *model = new OpcodeModel();
  static Pep10OpcodeInit ret(model);
  return model;
}

QString Pep10_ISA::objectCodeText() const { return _objectCodeText; }

void Pep10_ISA::setObjectCodeText(const QString &objectCodeText) {
  if (_objectCodeText == objectCodeText) return;
  _objectCodeText = objectCodeText;
  emit objectCodeTextChanged();
}

void Pep10_ISA::set(int abstraction, QString value) {
  using namespace builtins;
  if (abstraction == static_cast<int>(Abstraction::ISA3)) {
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

void Pep10_ISA::setCharIn(QString value) {
  if (_charIn == value) return;
  _charIn = value;
  emit charInChanged();
}

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

bool Pep10_ISA::isEmpty() const { return _objectCodeText.isEmpty(); }

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
  _memory->setSP(-1);
  _memory->setPC(-1, -1);
  _memory->clearModifiedAndUpdateGUI();
  return true;
}
bool Pep10_ISA::onFormatObject() {

  ObjectUtilities utils;
  utils.setBytesPerRow(16);
  auto fmt = utils.format(_objectCodeText, true);
  setObjectCodeText(fmt);
  return true;
}

bool Pep10_ISA::onExecute() {
  prepareSim();
  _state = State::NormalExec;
  _stepsSinceLastInteraction = 0;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  _system->bus()->trace(true);
  emit deferredExecution(sim::api2::trace::Action::Assert, project::StepEnableFlags::Continue);
  return true;
}

bool Pep10_ISA::onDebuggingStart() {
  prepareSim();
  _state = State::DebugPaused;
  _stepsSinceLastInteraction = 0;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  _system->bus()->trace(true);
  return true;
}

bool Pep10_ISA::onDebuggingContinue() {
  _state = State::DebugExec;
  _pendingPause = false;
  _stepsSinceLastInteraction = 0;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  emit deferredExecution(sim::api2::trace::Action::Break, project::StepEnableFlags::Continue);
  return true;
}

bool Pep10_ISA::onDebuggingPause() { return _pendingPause = true; }

bool Pep10_ISA::onDebuggingStop() {
  _system->bus()->trace(false);
  _state = State::Halted;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  return true;
}

bool Pep10_ISA::onISARemoveAllBreakpoints() { return false; }

bool Pep10_ISA::onISAStep() {
  _state = State::DebugExec;
  _stepsSinceLastInteraction = 0;
  _pendingPause = false;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  emit deferredExecution(sim::api2::trace::Action::Break, project::StepEnableFlags::Step);
  return true;
}

bool Pep10_ISA::onISAStepOver() { return false; }

bool Pep10_ISA::onISAStepInto() { return false; }

bool Pep10_ISA::onISAStepOut() { return false; }

bool Pep10_ISA::onClearCPU() {
  _system->cpu()->csrs()->clear(0);
  _system->cpu()->regs()->clear(0);
  // Reset trace buffer, since its content is now meaningless.
  _tb->clear();
  _flags->onUpdateGUI();
  _registers->onUpdateGUI();
  return true;
}

bool Pep10_ISA::onClearMemory() {
  _system->bus()->clear(0);
  // Reset trace buffer, since its content is now meaningless.
  _tb->clear();
  _memory->clearModifiedAndUpdateGUI();
  return true;
}

void Pep10_ISA::onDeferredExecution(sim::api2::trace::Action stopOn, project::StepEnableFlags::Value step) {
  auto pwrOff = _system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  auto from = _tb->cend();
  bool err = false;

  State newState = _state;
  switch (_state) {
  case State::Halted: newState = State::NormalExec; break;
  case State::NormalExec: break;
  case State::DebugPaused: [[fallthrough]];
  case State::DebugExec: newState = State::DebugExec; break;
  }
  if (newState != _state) {
    _state = newState;
    emit allowedDebuggingChanged();
    emit allowedStepsChanged();
  }

  // If we exceed the number of steps, do we allow another deferredExecution?
  bool allowsResume = false;
  try {
    auto ending = _system->currentTick();
    // TODO: need a function to check if we have pushed / popped stuff on the call stack.
    // std::function<bool(decltype(_system->currentTick()))> s;
    switch (step) {
    case project::StepEnableFlags::Step: ending += 1; break;
    case project::StepEnableFlags::StepInto: break;
    case project::StepEnableFlags::Continue:
      ending += 1000;
      allowsResume = true;
      break;
    default: break;
    }
    while (_system->currentTick() < ending && endpoint->at_end() && !_pendingPause) {
      _system->tick(sim::api2::Scheduler::Mode::Jump);
      for (const auto &event : _tb->events()) {
        if (event.action >= stopOn) _pendingPause = true;
      }
      _tb->clearEvents();
    }
  } catch (const sim::api2::memory::Error &e) {
    err = true;
    if (e.type() == sim::api2::memory::Error::Type::NeedsMMI) {
      std::cerr << "Ran out of MMI\n";
    } else std::cerr << "Memory error: " << e.what() << std::endl;
    // Handle illegal opcodes or program crashes.
  } catch (const std::logic_error &e) {
    err = true;
    std::cerr << e.what() << std::endl;
  }
  // Only terminates if something written to endpoint or there was an error
  if (endpoint->next_value().has_value() || err) {
    switch (_state) {
      _system->bus()->trace(false);
    case State::NormalExec:
      _state = State::Halted;
      emit allowedDebuggingChanged();
      emit allowedStepsChanged();
      break;
    case State::DebugExec: [[fallthrough]];
    case State::DebugPaused: onDebuggingStop(); break;
    default: break;
    }
  }
  // Trigger a BP if we exceed a resonable number of chained executions
  else if (_stepsSinceLastInteraction++ > 10) {
    _state = State::DebugPaused;
    emit allowedDebuggingChanged();
    emit allowedStepsChanged();
    emit message("Pausing, potential infinite loop detected.");
  }
  // Queued connection, so it will be evaluated after the display is updated.
  else if (allowsResume && !_pendingPause)
    emit deferredExecution(stopOn, step);
  else {
    _pendingPause = false;
    _state = State::DebugPaused;
    emit allowedDebuggingChanged();
    emit allowedStepsChanged();
  }
  prepareGUIUpdate(from);
}

void Pep10_ISA::prepareSim() {

  // Ensure latests changes to object code pane are reflected in simulator.
  onLoadObject();
  _system->init();
  _tb->clear();
  auto pwrOff = _system->output("pwrOff");
  auto charOut = _system->output("charOut");
  charOut->clear(0);
  pwrOff->clear(0);

  auto charIn = _system->input("charIn");
  charIn->clear(0);
  auto charInEndpoint = charIn->endpoint();
  for (int it = 0; it < _charIn.size(); it++) charInEndpoint->append_value(_charIn[it].toLatin1());
  _pendingPause = false;

  // Repaint CPU & Memory panes
  _flags->onUpdateGUI();
  _registers->onUpdateGUI();
  // _memory->onUpdateGUI();
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

project::DebugEnableFlags::DebugEnableFlags(QObject *parent) : QObject(parent) {}

project::StepEnableFlags::StepEnableFlags(QObject *parent) : QObject(parent) {}

Pep10_ASMB::Pep10_ASMB(QVariant delegate, QObject *parent) : Pep10_ISA(delegate, parent, false) {
  _osAsmText = cs6e_os();
  auto elfsys = make_asmb_system(_osAsmText);
  _elf = elfsys.elf;
  _system = elfsys.system;
  _system->bus()->setBuffer(&*_tb);
  bindToSystem();
  auto asUnique = std::make_unique<sim::api2::trace::ValueFilter<quint8>>(
      _system->cpu()->regs(), static_cast<quint8>(isa::Pep10::Register::PC));
  _breakpoints = asUnique.get();
  _tb->addFilter(std::move(asUnique));
}

void Pep10_ASMB::set(int abstraction, QString value) {
  using namespace builtins;
  if (abstraction == static_cast<int>(Abstraction::ASMB5)) {
    setUserAsmText(value);
  } else if (abstraction == static_cast<int>(Abstraction::OS4)) {
    setOSAsmText(value);
  }
}

QString Pep10_ASMB::userAsmText() const { return _userAsmText; }

void Pep10_ASMB::setUserAsmText(const QString &userAsmText) {
  if (_userAsmText == userAsmText) return;
  _userAsmText = userAsmText;
  emit userAsmTextChanged();
}

QString Pep10_ASMB::userList() const { return _userList; }

const QList<Error *> Pep10_ASMB::userListAnnotations() const {
  QList<Error *> ret;
  for (auto [line, str] : _userListAnnotations) ret.push_back(new Error{line, str});
  return ret;
}

QString Pep10_ASMB::osAsmText() const { return _osAsmText; }

void Pep10_ASMB::setOSAsmText(const QString &osAsmText) {
  if (_osAsmText == osAsmText) return;
  _osAsmText = osAsmText;
  emit osAsmTextChanged();
}
QString Pep10_ASMB::osList() const { return _osList; }

const QList<Error *> Pep10_ASMB::osListAnnotations() const {
  QList<Error *> ret;
  for (auto [line, str] : _osListAnnotations) ret.push_back(new Error{line, str});
  return ret;
}
const QList<Error *> Pep10_ASMB::errors() const {
  QList<Error *> ret;
  for (auto [line, str] : _errors) ret.push_back(new Error{line, str});
  return ret;
}

bool Pep10_ASMB::isEmpty() const { return _userAsmText.isEmpty(); }

project::Environment Pep10_ASMB::env() const {
  using namespace builtins;
  return {.arch = Architecture::PEP10, .level = Abstraction::ASMB5, .features = project::Features::None};
}

builtins::Architecture Pep10_ASMB::architecture() const { return builtins::Architecture::PEP10; }

builtins::Abstraction Pep10_ASMB::abstraction() const { return builtins::Abstraction::ASMB5; }

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

bool Pep10_ASMB::onDebuggingStart() {
  Pep10_ISA::onDebuggingStart();
  updatePCLine();
  return true;
}

static constexpr auto to_string = [](const QString &acc, const auto &pair) {
  return acc.isEmpty() ? pair.first : acc + "\n" + pair.first;
};
bool Pep10_ASMB::onAssemble(bool doLoad) {
  _userList = _osList = "";
  _userListAnnotations = _osListAnnotations = {};
  auto macroRegistry = cs6e_macros();
  helpers::AsmHelper helper(macroRegistry, _osAsmText);
  helper.setUserText(_userAsmText);
  auto ret = helper.assemble();
  _errors = helper.errorsWithLines();
  _userLines2Address = helper.address2Lines(false);
  _osLines2Address = helper.address2Lines(true);
  emit clearListingBreakpoints();
  emit errorsChanged();
  if (!ret) {
    emit message(utils::msg_asm_failed);
    setObjectCodeText("");
    emit listingChanged();
    return false;
  }

  auto user = helper.splitListing(false);
  _userList = std::accumulate(user.begin(), user.end(), QString(), to_string);
  for (auto it = 0; it < user.size(); it++)
    if (auto pair = user[it]; !pair.second.isEmpty()) _userListAnnotations.push_back({it, pair.second});
  auto os = helper.splitListing(true);
  _osList = std::accumulate(os.begin(), os.end(), QString(), to_string);
  for (auto it = 0; it < os.size(); it++)
    if (auto pair = os[it]; !pair.second.isEmpty()) _osListAnnotations.push_back({it, pair.second});
  emit listingChanged();

  auto userBytes = helper.bytes(false);
  QString objectCodeText = pas::ops::pepp::bytesToObject(userBytes, 16);
  if (doLoad) {
    _system->bus()->write(0, {userBytes.data(), std::size_t(userBytes.length())}, gs);
    _memory->onRepaintAddress(0, userBytes.length());
  }
  setObjectCodeText(objectCodeText);
  emit requestSourceBreakpoints();
  emit message(utils::msg_asm_success);
  return true;
}

bool Pep10_ASMB::onAssembleThenFormat() {
  _userList = _osList = "";
  _userListAnnotations = _osListAnnotations = {};
  auto macroRegistry = cs6e_macros();
  helpers::AsmHelper helper(macroRegistry, _osAsmText);
  helper.setUserText(_userAsmText);
  auto ret = helper.assemble();
  _errors = helper.errorsWithLines();
  _userLines2Address = helper.address2Lines(false);
  _osLines2Address = helper.address2Lines(false);
  emit clearListingBreakpoints();
  emit errorsChanged();
  if (!ret) {
    message(utils::msg_asm_failed);
    emit listingChanged();
  } else {
    auto source = helper.formattedSource(false);
    setUserAsmText(source.join("\n"));
    auto userBytes = helper.bytes(false);
    QString objectCodeText = pas::ops::pepp::bytesToObject(userBytes, 16);
    setObjectCodeText(objectCodeText);
  }
  emit requestSourceBreakpoints();
  auto user = helper.splitListing(false);
  _userList = std::accumulate(user.begin(), user.end(), QString(), to_string);
  for (auto it = 0; it < user.size(); it++)
    if (auto pair = user[it]; !pair.second.isEmpty()) _userListAnnotations.push_back({it, pair.second});
  auto os = helper.splitListing(true);
  _osList = std::accumulate(os.begin(), os.end(), QString(), to_string);
  for (auto it = 0; it < os.size(); it++)
    if (auto pair = os[it]; !pair.second.isEmpty()) _osListAnnotations.push_back({it, pair.second});
  emit listingChanged();
  return true;
}

void Pep10_ASMB::onModifyUserSource(int line, Action action) {
  if (auto address = _userLines2Address.source2Address(line); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyUserSource(line, action);
  if (auto list = _userLines2Address.source2List(line); list) emit modifyUserList(*list, action);
}

void Pep10_ASMB::onModifyOSSource(int line, Action action) {
  if (auto address = _osLines2Address.source2Address(line); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyOSSource(line, action);
  if (auto list = _osLines2Address.source2List(line); list) emit modifyOSList(*list, action);
}

void Pep10_ASMB::onModifyUserList(int line, Action action) {
  if (auto address = _userLines2Address.list2Address(line); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyUserList(line, action);
  if (auto src = _userLines2Address.list2Source(line); src) emit modifyUserSource(*src, action);
}

void Pep10_ASMB::onModifyOSList(int line, Action action) {
  if (auto address = _osLines2Address.list2Address(line); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyOSList(line, action);
  if (auto src = _osLines2Address.list2Source(line); src) emit modifyOSSource(*src, action);
}

void Pep10_ASMB::prepareSim() {
  _system->bus()->clear(0);
  if (!onAssemble(true)) return;
  _system->init();
  _tb->clear();
  auto pwrOff = _system->output("pwrOff");
  auto charOut = _system->output("charOut");
  charOut->clear(0);
  pwrOff->clear(0);
  _system->setBootFlags(false, true);

  auto charIn = _system->input("charIn");
  charIn->clear(0);
  auto charInEndpoint = charIn->endpoint();
  for (int it = 0; it < _charIn.size(); it++) charInEndpoint->append_value(_charIn[it].toLatin1());

  _pendingPause = false;

  // Repaint CPU
  _flags->onUpdateGUI();
  _registers->onUpdateGUI();
  _memory->clearModifiedAndUpdateGUI();
}

void Pep10_ASMB::prepareGUIUpdate(sim::api2::trace::FrameIterator from) {
  updatePCLine();
  Pep10_ISA::prepareGUIUpdate(from);
}

void Pep10_ASMB::updatePCLine() {
  auto pc = _system->cpu()->startingPC();
  if (auto userSrc = _userLines2Address.address2Source(pc); userSrc) emit modifyUserSource(*userSrc, Action::ScrollTo);
  if (auto userList = _userLines2Address.address2List(pc); userList) {
    emit switchTo(false);
    emit modifyUserList(*userList, Action::HighlightExclusive);
  }
  if (auto osSrc = _osLines2Address.address2Source(pc); osSrc) emit modifyOSSource(*osSrc, Action::ScrollTo);
  if (auto osList = _osLines2Address.address2List(pc); osList) {
    emit switchTo(true);
    emit modifyOSList(*osList, Action::HighlightExclusive);
  }
}

void Pep10_ASMB::updateBPAtAddress(quint32 address, Action action) {
  switch (action) {
  case ScintillaAsmEditBase::Action::ToggleBP:
    if (_breakpoints->contains(address)) _breakpoints->remove(address);
    else _breakpoints->insert(address);
    break;
  case ScintillaAsmEditBase::Action::AddBP: _breakpoints->insert(address); break;
  case ScintillaAsmEditBase::Action::RemoveBP: _breakpoints->remove(address); break;
  default: break;
  }
}

Error::Error(int line, QString error, QObject *parent) : QObject(parent), line(line), error(error) {}
