#include "./pep10.hpp"
#include <QQmlEngine>
#include <elfio/elfio.hpp>
#include "toolchain/pas/obj/common.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"
#include "cpu/formats.hpp"
#include "help/builtins/figure.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "enums/isa/pep10.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/broadcast/pubsub.hpp"
#include "sim/device/simple_bus.hpp"
#include "sim/trace2/buffers.hpp"
#include "targets/isa3/helpers.hpp"
#include "targets/isa3/system.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep9/isa3/cpu.hpp"
#include "text/editor/object.hpp"
#include "utils/bits/strings.hpp"
#include "utils/strings.hpp"

// Prevent WASM-ld error due to multiply defined symbol in static lib.
namespace {
auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};
}
using namespace Qt::StringLiterals;

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
        formatted = u"%1,%2"_s.arg(QString(mnemonicEnum.valueToKey((int)op.instr.mnemon)).toUpper(),
                                   QString(addressEnum.valueToKey((int)op.mode)).toLower());
      }
      model->appendRow(formatted, it);
    }
  }
};

struct Pep9OpcodeInit {
  explicit Pep9OpcodeInit(OpcodeModel *model) {
    static const auto mnemonicEnum = QMetaEnum::fromType<isa::Pep9::Mnemonic>();
    static const auto addressEnum = QMetaEnum::fromType<isa::Pep9::AddressingMode>();
    for (int it = 0; it < 256; it++) {
      auto op = isa::Pep9::opcodeLUT[it];
      if (!op.valid) continue;
      QString formatted;
      // instr.unary indicates if the instruction is hardware-unary (i.e., it could be a nonunary trap SCALL).
      // This is why we test the addressing mode instead, since nonunary traps will have an addressing mode.
      if (op.mode == isa::Pep9::AddressingMode::NONE) {
        formatted = QString(mnemonicEnum.valueToKey((int)op.instr.mnemon)).toUpper();
      } else {
        formatted = u"%1, %2"_s.arg(QString(mnemonicEnum.valueToKey((int)op.instr.mnemon)).toUpper(),
                                    QString(addressEnum.valueToKey((int)op.mode)).toLower());
      }
      model->appendRow(formatted, it);
    }
  }
};

struct SystemAssembly {
  QSharedPointer<ELFIO::elfio> elf;
  QSharedPointer<targets::isa::System> system;
};

QSharedPointer<macro::Registry> cs5e_macros() {
  auto book = helpers::book(5);
  return helpers::registry(book, {});
}

QSharedPointer<macro::Registry> cs6e_macros() {
  auto book = helpers::book(6);
  return helpers::registry(book, {});
}
SystemAssembly make_isa_system(project::Environment env) {
  using enum pepp::Architecture;
  QSharedPointer<const builtins::Book> book;
  QSharedPointer<macro::Registry> macroRegistry;
  QString osContents;
  switch (env.arch) {
  case PEP9: {
    book = helpers::book(5);
    auto os = book->findFigure("os", "pep9os");
    osContents = os->typesafeElements()["pep"]->contents();
    macroRegistry = cs5e_macros();
    break;
  }
  case PEP10: {
    book = helpers::book(6);
    auto os = book->findFigure("os", "pep10baremetal");
    osContents = os->typesafeElements()["pep"]->contents();
    macroRegistry = cs6e_macros();
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }

  helpers::AsmHelper helper(macroRegistry, osContents, env.arch);
  auto result = helper.assemble();
  if (!result) qWarning() << "Default OS assembly failed";

  SystemAssembly ret;
  // Need to reload to properly compute segment addresses.
  ret.elf = helper.elf();
  ret.system = targets::isa::systemFromElf(*ret.elf, true);
  return ret;
}

QString cs6e_bm() {
  auto book = helpers::book(6);
  auto os = book->findFigure("os", "pep10baremetal");
  return os->typesafeElements()["pep"]->contents();
}
QString cs6e_os() {
  auto book = helpers::book(6);
  auto os = book->findFigure("os", "pep10os");
  return os->typesafeElements()["pep"]->contents();
}

QString cs5e_os() {
  auto book = helpers::book(5);
  auto os = book->findFigure("os", "pep9os");
  return os->typesafeElements()["pep"]->contents();
}

// TODO: fix
SystemAssembly make_asmb_system(project::Environment env, QString os) {
  using enum pepp::Architecture;
  QSharedPointer<const builtins::Book> book;
  QSharedPointer<macro::Registry> macroRegistry;
  QString osContents;
  switch (env.arch) {
  case PEP9: {
    book = helpers::book(5);
    auto os = book->findFigure("os", "pep9os");
    osContents = os->typesafeElements()["pep"]->contents();
    macroRegistry = cs5e_macros();
    break;
  }
  case PEP10: {
    book = helpers::book(6);
    auto os = book->findFigure("os", "pep10baremetal");
    osContents = os->typesafeElements()["pep"]->contents();
    macroRegistry = cs6e_macros();
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }
  helpers::AsmHelper helper(macroRegistry, os, env.arch);
  auto result = helper.assemble();
  if (!result) {
    qWarning() << "Default OS assembly failed";
  }
  SystemAssembly ret;
  ret.elf = helper.elf();
  ret.system = targets::isa::systemFromElf(*ret.elf, true);
  return ret;
}

template <typename CPU, typename ISA>
RegisterModel *register_model(targets::isa::System *system, OpcodeModel *opcodes, QObject *parent = nullptr) {
  using RF = QSharedPointer<RegisterFormatter>;
  using TF = QSharedPointer<TextFormatter>;
  using AF = QSharedPointer<ASCIIFormatter>;
  using MF = QSharedPointer<MnemonicFormatter>;
  using HF = QSharedPointer<HexFormatter>;
  using SF = QSharedPointer<SignedDecFormatter>;
  using UF = QSharedPointer<UnsignedDecFormatter>;
  using BF = QSharedPointer<BinaryFormatter>;
  using OF = QSharedPointer<OptionalFormatter>;
  using VF = QSharedPointer<VariableByteLengthFormatter>;
  using VecF = QVector<RF>;
  auto ret = new RegisterModel(parent);
  auto _register = [](typename ISA::Register r, auto *system) {
    if (system == nullptr) return quint16{0};
    quint16 ret = 0;
    auto cpu = static_cast<CPU *>(system->cpu());
    targets::isa::readRegister<ISA>(cpu->regs(), r, ret, gs);
    return ret;
  };
  auto cpu = static_cast<CPU *>(system->cpu());
  // BUG: _system seems to be getting deleted very easily. It probably shouldn't be a shared pointer.
  // DO NOT CAPTURE _system INDIRECTLY VIA ANOTHER LAMBDA. It will crash.
  auto A = [=]() { return _register(ISA::Register::A, system); };
  auto X = [=]() { return _register(ISA::Register::X, system); };
  auto SP = [=]() { return _register(ISA::Register::SP, system); };
  auto PC = [=]() { return _register(ISA::Register::PC, system); };
  auto IS = [=]() { return _register(ISA::Register::IS, system); };
  auto IS_TEXT = [=]() {
    int opcode = _register(ISA::Register::IS, system);
    auto row = opcodes->indexFromOpcode(opcode);
    return opcodes->data(opcodes->index(row), Qt::DisplayRole).toString();
  };
  auto OS = [=]() { return _register(ISA::Register::OS, system); };
  auto notU = [=]() {
    auto is = _register(ISA::Register::IS, system);
    auto op = ISA::opcodeLUT[is];
    return !op.instr.unary;
  };
  auto operand = [=]() { return cpu->currentOperand().value_or(0); };
  auto cf = [](std::function<int64_t()> reg, qsizetype index) {
    return QSharedPointer<ChoiceFormatter>::create(
        VecF{SF::create(reg, 2), UF::create(reg, 2), BF::create(reg, 2), AF::create(reg, 2)}, index);
  };
  ret->appendFormatters({TF::create("Accumulator"), HF::create(A, 2), cf(A, 0)});
  ret->appendFormatters({TF::create("Index Register"), HF::create(X, 2), cf(X, 0)});
  ret->appendFormatters({TF::create("Stack Pointer"), HF::create(SP, 2), cf(SP, 1)});
  ret->appendFormatters({TF::create("Program Counter"), HF::create(PC, 2), cf(PC, 1)});
  ret->appendFormatters({TF::create("Instruction Specifier"), BF::create(IS, 1), MF::create(IS_TEXT)});
  // OS display format changes between signed & unsigned depending on if adressing mode is I or stack-relative.
  auto os_signed = SF::create(OS, 2);
  auto os_unsigned = UF::create(OS, 2);
  auto os_chooser = [=]() {
    auto is = _register(ISA::Register::IS, system);
    return (qsizetype)ISA::decodeOperandAsSigned(is);
  };
  auto os_choice = QSharedPointer<AutoChoice>::create(VecF{os_unsigned, os_signed}, os_chooser);
  ret->appendFormatters(
      {TF::create("Operand Specifier"), OF::create(HF::create(OS, 2), notU), OF::create(os_choice, notU)});
  auto length = [=]() {
    auto is = _register(ISA::Register::IS, system);
    return ISA::operandBytes(is);
  };
  auto opr_hex = HF::create(operand, 2);
  auto opr_dec = SF::create(operand, 2);
  auto opr_hex_wrapped = VF::create(OF::create(opr_hex, notU), length, 2);
  auto opr_dec_wrapped = VF::create(OF::create(opr_dec, notU), length, 2);
  ret->appendFormatters({TF::create("(Operand)"), opr_hex_wrapped, opr_dec_wrapped});
  return ret;
}

template <typename CPU, typename ISA> FlagModel *flag_model(targets::isa::System *system, QObject *parent = nullptr) {
  using F = QSharedPointer<Flag>;
  auto ret = new FlagModel(parent);
  auto _flag = [](typename ISA::CSR s, auto *system) {
    if (system == nullptr) return false;
    bool ret = 0;
    auto cpu = static_cast<CPU *>(system->cpu());
    targets::isa::readCSR<ISA>(cpu->csrs(), s, ret, gs);
    return ret;
  };
  auto cpu = static_cast<targets::pep10::isa::CPU *>(system->cpu());
  // See above for wanings on _system pointer.
  auto N = [=]() { return _flag(ISA::CSR::N, system); };
  auto Z = [=]() { return _flag(ISA::CSR::Z, system); };
  auto V = [=]() { return _flag(ISA::CSR::V, system); };
  auto C = [=]() { return _flag(ISA::CSR::C, system); };
  ret->appendFlag({F::create("N", N)});
  ret->appendFlag({F::create("Z", Z)});
  ret->appendFlag({F::create("V", V)});
  ret->appendFlag({F::create("C", C)});
  return ret;
}

Pep_ISA::Pep_ISA(project::Environment env, QObject *parent, bool initializeSystem)
    : QObject(parent), _env(env), _tb(QSharedPointer<sim::trace2::InfiniteBuffer>::create()), _memory(nullptr),
      _registers(nullptr), _flags(nullptr) {
  _system.clear();
  assert(_system.isNull());
  _dbg = QSharedPointer<pepp::debug::Debugger>::create(this);
  if (initializeSystem) {
    auto elfsys = make_isa_system(env);
    _elf = elfsys.elf;
    _system = elfsys.system;
    _system->bus()->setBuffer(&*_tb);
    bindToSystem();
  }
  connect(this, &Pep_ISA::deferredExecution, this, &Pep_ISA::onDeferredExecution, Qt::QueuedConnection);
}

void Pep_ISA::bindToSystem() {
  using enum pepp::Architecture;
  switch (_env.arch) {
  case PEP9:
    _flags = flag_model<targets::pep9::isa::CPU, isa::Pep9>(&*_system, this);
    _registers = register_model<targets::pep9::isa::CPU, isa::Pep9>(&*_system, mnemonics(), this);
    break;
  case PEP10:
    _flags = flag_model<targets::pep10::isa::CPU, isa::Pep10>(&*_system, this);
    _registers = register_model<targets::pep10::isa::CPU, isa::Pep10>(&*_system, mnemonics(), this);
    break;
  default: throw std::logic_error("Unimplemented");
  }
  // Use old-style connections to avoid a linker error in WASM.
  // For some reason, new-style connects cause LD to insert a 0-arg updateGUI into the object file.
  // We can defeat the linker with the following Qt macros.
  connect(this, SIGNAL(updateGUI(sim::api2::trace::FrameIterator)), _flags, SLOT(onUpdateGUI()));
  QQmlEngine::setObjectOwnership(_flags, QQmlEngine::CppOwnership);
  connect(this, SIGNAL(updateGUI(sim::api2::trace::FrameIterator)), _registers, SLOT(onUpdateGUI()));
  QQmlEngine::setObjectOwnership(_registers, QQmlEngine::CppOwnership);

  using TMAS = sim::trace2::TranslatingModifiedAddressSink<quint16>;
  auto sink = QSharedPointer<TMAS>::create(_system->pathManager(), _system->bus());

  _memory = new SimulatorRawMemory(_system->bus(), sink, this);
  connect(this, SIGNAL(updateGUI(sim::api2::trace::FrameIterator)), _memory,
          SLOT(onUpdateGUI(sim::api2::trace::FrameIterator)));
  QQmlEngine::setObjectOwnership(_memory, QQmlEngine::CppOwnership);
}

project::Environment Pep_ISA::env() const {
  using namespace builtins;
  return _env;
}

pepp::Architecture Pep_ISA::architecture() const { return _env.arch; }

pepp::Abstraction Pep_ISA::abstraction() const { return _env.level; }

QString Pep_ISA::delegatePath() const { return "qrc:/qt/qml/edu/pepp/project/Pep10ISA.qml"; }

ARawMemory *Pep_ISA::memory() const { return _memory; }

OpcodeModel *Pep_ISA::mnemonics() const {
  using enum pepp::Architecture;
  switch (_env.arch) {
  case PEP9: {
    static OpcodeModel *model = new OpcodeModel();
    static Pep9OpcodeInit ret(model);
    return model;
  }
  case PEP10: {
    static OpcodeModel *model = new OpcodeModel();
    static Pep10OpcodeInit ret(model);
    return model;
  }
  default: throw std::logic_error("Unimplemented");
  }
}

QString Pep_ISA::objectCodeText() const { return _objectCodeText; }

void Pep_ISA::setObjectCodeText(const QString &objectCodeText) {
  if (_objectCodeText == objectCodeText) return;
  _objectCodeText = objectCodeText;
  emit objectCodeTextChanged();
}

void Pep_ISA::set(int abstraction, QString value) {
  using enum pepp::Abstraction;
  if (abstraction == static_cast<int>(ISA3)) {
    setObjectCodeText(value);
  }
}

pepp::debug::BreakpointSet *Pep_ISA::breakpointModel() {
  auto *bp = _dbg->bps.get();
  QQmlEngine::setObjectOwnership(bp, QQmlEngine::CppOwnership);
  return bp;
}

int Pep_ISA::allowedDebugging() const {
  using D = project::DebugEnableFlags;
  switch (_state) {
  case State::Halted: return D::Start | D::LoadObject | D::Execute;
  case State::DebugPaused: return D::Continue | D::Stop;
  case State::NormalExec: return D::Stop;
  case State::DebugExec: return D::Stop;
  default: return 0b0;
  }
}

int Pep_ISA::allowedSteps() const {
  using enum pepp::Architecture;
  if (_state != State::DebugPaused) return 0b0;
  using S = project::StepEnableFlags::Value;
  // TODO: have CPU tell you if next instr can step into.
  quint8 is = 0;
  quint16 pc = 0;
  bool isCall = false;
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, pc, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    isCall = isa::Pep9::isCall(is);
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, pc, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    isCall = isa::Pep10::isCall(is);
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }
  if (isCall) return S::Step | S::StepOver | S::StepOut | S::StepInto;
  return S::Step | S::StepOver | S::StepOut;
}

QString Pep_ISA::charIn() const { return _charIn; }

void Pep_ISA::setCharIn(QString value) {
  if (_charIn == value) return;
  _charIn = value;
  emit charInChanged();
}

QString Pep_ISA::charOut() const {
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
  using namespace Qt::StringLiterals;
  return u""_s;
}

bool Pep_ISA::isEmpty() const { return _objectCodeText.isEmpty(); }

uint8_t Pep_ISA::read_mem_u8(uint32_t address) const {
  if (_system == nullptr) return 0;
  quint8 temp = 0;
  _system->bus()->read((uint16_t)address, {&temp, 1}, gs);
  return temp;
}

uint16_t Pep_ISA::read_mem_u16(uint32_t address) const {
  if (_system == nullptr) return 0;
  quint16 temp = 0;
  _system->bus()->read((uint16_t)address, {(quint8 *)&temp, 2}, gs);
  if (bits::hostOrder() != bits::Order::BigEndian) temp = bits::byteswap(temp);
  return temp;
}

pepp::debug::TypedBits Pep_ISA::evaluate_variable(QStringView name) const {
  using T = pepp::debug::ExpressionType;
  return {.allows_address_of = true, .type = T::i16, .bits = (uint64_t)name.length()};
}

uint32_t Pep_ISA::cache_debug_variable_name(QStringView name) const {
  static const auto meta = QMetaEnum::fromType<Pep_ISA::DebugVariables>();
  auto upper = name.toString().toUpper();
  bool okay = false;
  auto v = meta.keyToValue(upper.toStdString().c_str(), &okay);
  if (!okay) return -1;
  return v;
}

pepp::debug::TypedBits Pep_ISA::evaluate_debug_variable(uint32_t cache_id) const {
  using T = pepp::debug::ExpressionType;
  using DV = Pep_ISA::DebugVariables;
  if (_system == nullptr) return {.allows_address_of = true, .type = T::i16, .bits = (uint64_t)0};
  uint16_t reg16;
  auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
  switch (cache_id) {
  case static_cast<uint32_t>(DV::A):
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::A, reg16, gs);
    return pepp::debug::from_int((int16_t)reg16);
  case static_cast<uint32_t>(DV::X):
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::X, reg16, gs);
    return pepp::debug::from_int((int16_t)reg16);
  case static_cast<uint32_t>(DV::SP):
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, reg16, gs);
    return pepp::debug::from_int(reg16);
  case static_cast<uint32_t>(DV::PC):
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, reg16, gs);
    return pepp::debug::from_int(reg16);
  case static_cast<uint32_t>(DV::IS):
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::IS, reg16, gs);
    return pepp::debug::from_int((int8_t)reg16);
  case static_cast<uint32_t>(DV::OS):
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::OS, reg16, gs);
    return pepp::debug::from_int(reg16);
  default: return {.allows_address_of = true, .type = T::i16, .bits = (uint64_t)0};
  }
}

bool Pep_ISA::onSaveCurrent() { return false; }

bool Pep_ISA::onLoadObject() {
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
  targets::isa::loadElfSegments(*bus, *_elf);
  // Load user program into memory.
  bus->write(0, *bytes, gs);
  // Update cpu-dependent fields in memory before triggering a GUI update.
  quint8 is;
  quint16 sp, pc;
  bool isUnary;
  using enum pepp::Architecture;
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, pc, gs);
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, sp, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    isUnary = isa::Pep9::opcodeLUT[is].instr.unary;
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, pc, gs);
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, sp, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    isUnary = isa::Pep10::opcodeLUT[is].instr.unary;
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }

  _memory->setSP(sp);
  _memory->setPC(pc, pc + (isUnary ? 0 : 2));
  _memory->clearModifiedAndUpdateGUI();
  return true;
}

bool Pep_ISA::onFormatObject() {
  ObjectUtilities utils;
  utils.setBytesPerRow(16);
  auto includeZZ = architecture() != pepp::Architecture::PEP10;
  auto fmt = utils.format(_objectCodeText, includeZZ);
  setObjectCodeText(fmt);
  return true;
}

bool Pep_ISA::onExecute() {
  prepareSim();
  _state = State::NormalExec;
  _stepsSinceLastInteraction = 0;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  _system->bus()->trace(true);
  emit deferredExecution([]() { return false; });
  return true;
}

bool Pep_ISA::onDebuggingStart() {
  prepareSim();
  _state = State::DebugPaused;
  _stepsSinceLastInteraction = 0;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  _system->bus()->trace(true);
  return true;
}

bool Pep_ISA::onDebuggingContinue() {
  _state = State::DebugExec;
  _pendingPause = false;
  _stepsSinceLastInteraction = 0;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  emit deferredExecution([]() { return false; });
  return true;
}

bool Pep_ISA::onDebuggingPause() { return _pendingPause = true; }

bool Pep_ISA::onDebuggingStop() {
  _system->bus()->trace(false);
  _state = State::Halted;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  return true;
}

bool Pep_ISA::onISARemoveAllBreakpoints() {
  _dbg->bps->clearBPs();
  return true;
}

template <typename CPU, typename ISA> auto generateStepCondition(targets::isa::System *system, qint16 offset) {
  auto cpu = static_cast<CPU *>(system->cpu());
  auto targetDepth = std::clamp<quint32>(cpu->depth() + (qint32)offset, 0, 0xffff);
  auto ret = [cpu, targetDepth]() { return cpu->depth() <= targetDepth; };
  return std::function<bool(void)>(ret);
}

bool Pep_ISA::onISAStep() {
  using enum pepp::Architecture;
  bool nextIsTrap = false;
  quint8 is;
  quint16 pc;
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, pc, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    nextIsTrap = isa::Pep9::isTrap(is);
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, pc, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    nextIsTrap = isa::Pep10::isTrap(is);
    break;
  }
  default: throw std::logic_error("Unimplemented architecture");
  }

  if (nextIsTrap) return onISAStepOver();
  else return onISAStepInto();
}

bool Pep_ISA::onISAStepOver() { return stepDepthHelper(0); }

bool Pep_ISA::onISAStepInto() { return stepDepthHelper(1); }

bool Pep_ISA::onISAStepOut() { return stepDepthHelper(-1); }

bool Pep_ISA::onClearCPU() {
  using enum pepp::Architecture;
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    cpu->csrs()->clear(0);
    cpu->regs()->clear(0);
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    cpu->csrs()->clear(0);
    cpu->regs()->clear(0);
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }

  // Reset trace buffer, since its content is now meaningless.
  _tb->clear();
  _flags->onUpdateGUI();
  _registers->onUpdateGUI();
  return true;
}

bool Pep_ISA::onClearMemory() {
  _system->bus()->clear(0);
  // Reset trace buffer, since its content is now meaningless.
  _tb->clear();
  _memory->clearModifiedAndUpdateGUI();
  return true;
}

void Pep_ISA::onDeferredExecution(std::function<bool()> step) {
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
  bool allowsResume = true;
  try {
    auto ending = _system->currentTick() + 1000;
    do {
      _system->tick(sim::api2::Scheduler::Mode::Jump);
      if (_dbg->bps->hit()) {
        _dbg->bps->clearHit();
        _pendingPause = true;
      }
      _pendingPause |= step();
    } while (_system->currentTick() < ending && endpoint->at_end() && !_pendingPause);
  } catch (const sim::api2::memory::Error &e) {
    err = true;
    if (e.type() == sim::api2::memory::Error::Type::NeedsMMI) {
      emit message("Ran out of MMI");
    } else std::cerr << "Memory error: " << e.what() << std::endl;
    // Handle illegal opcodes or program crashes.
  } catch (const std::logic_error &e) {
    err = true;
    emit message(e.what());
  }
  // Only terminates if something written to endpoint or there was an error
  if (endpoint->next_value().has_value() || err) {
    switch (_state) {
    case State::NormalExec:
      _system->bus()->trace(false);
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
    emit deferredExecution(step);
  else {
    _pendingPause = false;
    _state = State::DebugPaused;
    emit allowedDebuggingChanged();
    emit allowedStepsChanged();
  }
  prepareGUIUpdate(from);
}

void Pep_ISA::prepareSim() {
  // Ensure latests changes to object code pane are reflected in simulator.
  onLoadObject();
  _system->init();
  _tb->clear();
  _dbg->bps->clearHit();
  auto pwrOff = _system->output("pwrOff");
  auto charOut = _system->output("charOut");
  charOut->clear(0);
  pwrOff->clear(0);
  emit charOutChanged();

  auto charIn = _system->input("charIn");
  charIn->clear(0);
  auto charInEndpoint = charIn->endpoint();
  for (int it = 0; it < _charIn.size(); it++) charInEndpoint->append_value(_charIn[it].toLatin1());
  _pendingPause = false;

  // Repaint CPU & Memory panes
  _flags->onUpdateGUI();
  _registers->onUpdateGUI();
  updateMemPCSP();
  _memory->clearModifiedAndUpdateGUI();
  //_memory->onUpdateGUI();
}

void Pep_ISA::prepareGUIUpdate(sim::api2::trace::FrameIterator from) {
  updateMemPCSP();
  emit charOutChanged();
  emit updateGUI(from);
}

void Pep_ISA::updateMemPCSP() const {
  using enum pepp::Architecture;
  quint8 is;
  quint16 sp, pc;
  bool isUnary;
  // Update cpu-dependent fields in memory before triggering a GUI update.
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, pc, gs);
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, sp, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    isUnary = isa::Pep9::opcodeLUT[is].instr.unary;
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, pc, gs);
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, sp, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    isUnary = isa::Pep10::opcodeLUT[is].instr.unary;
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }

  _memory->setSP(sp);
  _memory->setPC(pc, pc + (isUnary ? 0 : 2));
}

bool Pep_ISA::stepDepthHelper(qint16 offset) {
  using enum pepp::Architecture;
  _state = State::DebugExec;
  _stepsSinceLastInteraction = 0;
  _pendingPause = false;
  emit allowedDebuggingChanged();
  emit allowedStepsChanged();
  switch (_system->architecture()) {
  case PEP9:
    emit deferredExecution(generateStepCondition<targets::pep9::isa::CPU, isa::Pep9>(&*_system, offset));
    break;
  case PEP10:
    emit deferredExecution(generateStepCondition<targets::pep10::isa::CPU, isa::Pep10>(&*_system, offset));
    break;
  default: throw std::logic_error("Unimplemented architecture");
  }
  return true;
}

project::DebugEnableFlags::DebugEnableFlags(QObject *parent) : QObject(parent) {}

project::StepEnableFlags::StepEnableFlags(QObject *parent) : QObject(parent) {}

Pep_ASMB::Pep_ASMB(project::Environment env, QObject *parent) : Pep_ISA(env, parent, false) {
  using enum pepp::Architecture;
  using enum pepp::Abstraction;
  switch (_env.arch) {
  case PEP9: _osAsmText = cs5e_os(); break;
  case PEP10:
    if (_env.level == ASMB3) _osAsmText = cs6e_bm();
    else _osAsmText = cs6e_os();
    break;
  default: throw std::logic_error("Unimplemented architecture");
  }

  auto elfsys = make_asmb_system(_env, _osAsmText);
  _elf = elfsys.elf;
  _system = elfsys.system;
  _system->bus()->setBuffer(&*_tb);
  bindToSystem();
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    static const quint8 PC = 2 * static_cast<quint8>(isa::Pep9::Register::PC);
    cpu->setBuffer(&*_tb);
    cpu->setDebugger(&*_dbg);
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    static const quint8 PC = 2 * static_cast<quint8>(isa::Pep10::Register::PC);
    cpu->setBuffer(&*_tb);
    cpu->setDebugger(&*_dbg);
    break;
  }
  default: throw std::logic_error("Unimplemented architecture");
  }
  auto bps = _dbg->bps.get();
  connect(bps, &pepp::debug::BreakpointSet::conditionChanged, this, &Pep_ASMB::onBPConditionChanged);
  // Scopes must be present to allow adding BPs on source programs before assembly
  _dbg->line_maps->addScope("user");
  _dbg->line_maps->addScope("os");
}

QString Pep_ASMB::delegatePath() const { return "qrc:/qt/qml/edu/pepp/project/Pep10ASMB.qml"; }

void Pep_ASMB::set(int abstraction, QString value) {
  using enum pepp::Abstraction;
  using namespace builtins;
  if (abstraction == static_cast<int>(ASMB5) || abstraction == static_cast<int>(ASMB3)) {
    setUserAsmText(value);
  } else if (abstraction == static_cast<int>(OS4)) {
    setOSAsmText(value);
  }
}

QString Pep_ASMB::userAsmText() const { return _userAsmText; }

void Pep_ASMB::setUserAsmText(const QString &userAsmText) {
  if (_userAsmText == userAsmText) return;
  _userAsmText = userAsmText;
  emit userAsmTextChanged();
}

QString Pep_ASMB::userList() const { return _userList; }

QString Pep_ASMB::osAsmText() const { return _osAsmText; }

void Pep_ASMB::setOSAsmText(const QString &osAsmText) {
  if (_osAsmText == osAsmText) return;
  _osAsmText = osAsmText;
  emit osAsmTextChanged();
}
QString Pep_ASMB::osList() const { return _osList; }

const QList<Error *> Pep_ASMB::errors() const {
  QList<Error *> ret;
  for (auto [line, str] : _errors) ret.push_back(new Error{line, str});
  return ret;
}

bool Pep_ASMB::isEmpty() const { return _userAsmText.isEmpty(); }

StaticSymbolModel *Pep_ASMB::staticSymbolModel() const { return _dbg->static_symbol_model.get(); }

pepp::debug::WatchExpressionEditor *Pep_ASMB::watchExpressions() const { return _dbg->watch_expressions.get(); }

ScopedLines2Addresses *Pep_ASMB::line2addr() const { return _dbg->line_maps.get(); }

int Pep_ASMB::allowedDebugging() const {
  using D = project::DebugEnableFlags;
  switch (_state) {
  case State::Halted: return D::Start | D::Execute;
  case State::DebugPaused: return D::Continue | D::Stop;
  case State::NormalExec: return D::Stop;
  case State::DebugExec: return D::Stop;
  default: return 0b0;
  }
}


bool Pep_ASMB::onDebuggingStart() {
  Pep_ISA::onDebuggingStart();
  updatePCLine();
  return true;
}

static constexpr auto to_string = [](const QString &acc, const auto &line) {
  return acc.isEmpty() ? line : acc + "\n" + line;
};

bool Pep_ASMB::onAssemble(bool doLoad) {
  using enum pepp::Architecture;
  _userList = _osList = "";
  QSharedPointer<macro::Registry> macroRegistry = nullptr;
  switch (_env.arch) {
  case PEP9: macroRegistry = cs5e_macros(); break;
  case PEP10: macroRegistry = cs6e_macros(); break;
  default: throw std::logic_error("Unimplemented architecture");
  }

  helpers::AsmHelper helper(macroRegistry, _osAsmText, _env.arch);
  helper.setUserText(_userAsmText);
  auto ret = helper.assemble();
  _errors = helper.errorsWithLines();
  emit clearListingBreakpoints();
  emit errorsChanged();
  if (!ret) {
    emit message(utils::msg_asm_failed);
    setObjectCodeText("");
    _dbg->static_symbol_model->clearData();

    emit listingChanged();
    return false;
  }
  auto elf = helper.elf();
  _dbg->line_maps->onReset();
  _dbg->line_maps->addScope("user", std::move(helper.address2Lines(false)));
  _dbg->line_maps->addScope("os", std::move(helper.address2Lines(true)));
  _dbg->static_symbol_model->setFromElf(elf.get());
  auto user = helper.listing(false), os = helper.listing(true);
  _userList = std::accumulate(user.begin(), user.end(), QString(), to_string);
  _osList = std::accumulate(os.begin(), os.end(), QString(), to_string);

  emit listingChanged();

  auto userBytes = helper.bytes(false);
  QString objectCodeText = pas::ops::pepp::bytesToObject(userBytes, 16, architecture() != pepp::Architecture::PEP10);

  _system->reconfigure(*elf);
  if (doLoad) _system->bus()->write(0, {userBytes.data(), std::size_t(userBytes.length())}, gs);

  // Hack to help debugger understand callViaRet.
  auto callViaRet = helper.callViaRets();
  switch (_env.arch) {
  case PEP10: {
    if (auto cpu = dynamic_cast<targets::pep10::isa::CPU *>(_system->cpu()); cpu) cpu->setCallsViaRet(callViaRet);
    break;
  }
  default: throw std::logic_error("Unimplemented architecture");
  }

  setObjectCodeText(objectCodeText);
  emit requestSourceBreakpoints();
  emit message(utils::msg_asm_success);
  return true;
}

bool Pep_ASMB::onAssembleThenLoad() {
  _system->bus()->clear(0);
  onAssemble(true);
  _system->doReloadEntries();
  _memory->clearModifiedAndUpdateGUI();
  _tb->clear();
  return true;
}

bool Pep_ASMB::onAssembleThenFormat() {
  using enum pepp::Architecture;
  _userList = _osList = "";
  QSharedPointer<macro::Registry> macroRegistry = nullptr;
  switch (_env.arch) {
  case PEP9: macroRegistry = cs5e_macros(); break;
  case PEP10: macroRegistry = cs6e_macros(); break;
  default: throw std::logic_error("Unimplemented architecture");
  }
  helpers::AsmHelper helper(macroRegistry, _osAsmText, _env.arch);
  helper.setUserText(_userAsmText);
  auto ret = helper.assemble();
  _errors = helper.errorsWithLines();
  _dbg->line_maps->onReset();
  _dbg->line_maps->addScope("user", helper.address2Lines(false));
  _dbg->line_maps->addScope("os", helper.address2Lines(true));
  emit clearListingBreakpoints();
  emit errorsChanged();
  if (!ret) {
    message(utils::msg_asm_failed);
    emit listingChanged();
  } else {
    auto source = helper.formattedSource(false);
    setUserAsmText(source.join("\n"));
    auto userBytes = helper.bytes(false);
    QString objectCodeText = pas::ops::pepp::bytesToObject(userBytes, 16, architecture() != pepp::Architecture::PEP10);
    setObjectCodeText(objectCodeText);
  }
  emit requestSourceBreakpoints();
  auto user = helper.listing(false), os = helper.listing(true);
  _userList = std::accumulate(user.begin(), user.end(), QString(), to_string);
  _osList = std::accumulate(os.begin(), os.end(), QString(), to_string);
  emit listingChanged();
  return true;
}

void Pep_ASMB::onModifyUserSource(int line, Action action) {
  auto maybe_scope = _dbg->line_maps->name2scope("user");
  if (!maybe_scope) return;
  auto scope = *maybe_scope;
  if (auto address = _dbg->line_maps->source2Address(line, scope); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyUserSource(line, action);
  if (auto list = _dbg->line_maps->source2List(line, scope); list) emit modifyUserList(*list, action);
}

void Pep_ASMB::onModifyOSSource(int line, Action action) {
  auto maybe_scope = _dbg->line_maps->name2scope("os");
  if (!maybe_scope) return;
  auto scope = *maybe_scope;
  if (auto address = _dbg->line_maps->source2Address(line, scope); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyOSSource(line, action);
  if (auto list = _dbg->line_maps->source2List(line, scope); list) emit modifyOSList(*list, action);
}

void Pep_ASMB::onModifyUserList(int line, Action action) {
  auto maybe_scope = _dbg->line_maps->name2scope("user");
  if (!maybe_scope) return;
  auto scope = *maybe_scope;
  if (auto address = _dbg->line_maps->list2Address(line, scope); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyUserList(line, action);
  if (auto src = _dbg->line_maps->list2Source(line, scope); src) emit modifyUserSource(*src, action);
}

void Pep_ASMB::onModifyOSList(int line, Action action) {
  auto maybe_scope = _dbg->line_maps->name2scope("os");
  if (!maybe_scope) return;
  auto scope = *maybe_scope;
  if (auto address = _dbg->line_maps->list2Address(line, scope); address && action != Action::ScrollTo)
    updateBPAtAddress(*address, action);
  emit modifyOSList(line, action);
  if (auto src = _dbg->line_maps->list2Source(line, scope); src) emit modifyOSSource(*src, action);
}

void Pep_ASMB::onBPConditionChanged(quint16 address, bool conditional) {
  auto maybe_os = _dbg->line_maps->name2scope("os");
  auto maybe_user = _dbg->line_maps->name2scope("user");
  auto action = conditional ? Action::MakeConditional : Action::MakeUnconditional;
  if (!maybe_os || !maybe_user) return;
  auto os = *maybe_os, user = *maybe_user;
  if (auto maybeList = _dbg->line_maps->address2List(address); maybeList) {
    auto [scope, line] = *maybeList;
    if (scope == os) emit modifyOSList(line, action);
    else if (scope == user) emit modifyUserList(line, action);
  }

  if (auto maybeSrc = _dbg->line_maps->address2Source(address); maybeSrc) {
    auto [scope, line] = *maybeSrc;
    if (scope == os) emit modifyOSSource(line, action);
    else if (scope == user) emit modifyUserSource(line, action);
  }
}

void Pep_ASMB::prepareSim() {
  using enum pepp::Architecture;
  // Must assemble first, otherwise we might load an outdated version of the OS
  // via _system->init(). Specifically, outdated memory-mapped vector values.
  if (!onAssemble(true)) return;
  _system->bus()->clear(0);
  _system->init();
  _tb->clear();
  _dbg->bps->clearHit();
  auto pwrOff = _system->output("pwrOff");
  auto charOut = _system->output("charOut");
  charOut->clear(0);
  pwrOff->clear(0);
  emit charOutChanged();

  auto charIn = _system->input("charIn");
  charIn->clear(0);
  auto charInEndpoint = charIn->endpoint();
  for (int it = 0; it < _charIn.size(); it++) charInEndpoint->append_value(_charIn[it].toLatin1());

  _pendingPause = false;

  quint8 is;
  quint16 sp, pc;
  bool isUnary;
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, pc, gs);
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, sp, gs);
    _system->bus()->read(pc, {&is, 1}, gs);

    isUnary = isa::Pep9::opcodeLUT[is].instr.unary;
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, pc, gs);
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, sp, gs);
    _system->bus()->read(pc, {&is, 1}, gs);
    isUnary = isa::Pep10::opcodeLUT[is].instr.unary;
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }
  // Update cpu-dependent fields in memory before triggering a GUI update.
  _memory->setSP(sp);
  _memory->setPC(pc, pc + (isUnary ? 0 : 2));
  _memory->clearModifiedAndUpdateGUI();
  // Repaint CPU
  _flags->onUpdateGUI();
  _registers->onUpdateGUI();
  _dbg->watch_expressions->onSimulationStart();
}

void Pep_ASMB::prepareGUIUpdate(sim::api2::trace::FrameIterator from) {
  updatePCLine();
  Pep_ISA::prepareGUIUpdate(from);
}

void Pep_ASMB::updatePCLine() {
  using enum pepp::Architecture;
  quint16 pc = 0;
  switch (_env.arch) {
  case PEP9: {
    auto cpu = static_cast<targets::pep9::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, pc, gs);
    break;
  }
  case PEP10: {
    auto cpu = static_cast<targets::pep10::isa::CPU *>(_system->cpu());
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, pc, gs);
    break;
  }
  default: throw std::logic_error("Unimplemented");
  }
  auto maybe_user_scope = _dbg->line_maps->name2scope("user");
  auto maybe_os_scope = _dbg->line_maps->name2scope("os");
  auto srcLine = _dbg->line_maps->address2Source(pc);
  auto listLine = _dbg->line_maps->address2List(pc);
  if (!maybe_user_scope || !maybe_os_scope) return;
  auto userScope = *maybe_user_scope, osScope = *maybe_os_scope;
  if (srcLine) {
    auto [scope, line] = *srcLine;
    if (scope == userScope) emit modifyUserSource(line, Action::ScrollTo);
    else if (scope == osScope) emit modifyOSSource(line, Action::ScrollTo);
  }
  if (listLine) {
    auto [scope, line] = *listLine;
    if (scope == userScope) {
      emit switchTo(false);
      emit modifyUserList(line, Action::HighlightExclusive);
    } else if (scope == osScope) {
      emit switchTo(true);
      emit modifyOSList(line, Action::HighlightExclusive);
    }
  }
}

void Pep_ISA::updateBPAtAddress(quint32 address, Action action) {

  auto as_quint16 = static_cast<quint16>(address);
  switch (action) {
  case ScintillaAsmEditBase::Action::ToggleBP:
    if (_dbg->bps->hasBP(as_quint16)) _dbg->bps->removeBP(as_quint16);
    else _dbg->bps->addBP(as_quint16);
    break;
  case ScintillaAsmEditBase::Action::AddBP: _dbg->bps->addBP(as_quint16); break;
  case ScintillaAsmEditBase::Action::RemoveBP: _dbg->bps->removeBP(as_quint16); break;
  default: break;
  }
}

Error::Error(int line, QString error, QObject *parent) : QObject(parent), line(line), error(error) {}
