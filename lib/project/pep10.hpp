/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <QQmlEngine>
#include <QStringListModel>
#include <qabstractitemmodel.h>
#include "aproject.hpp"
#include "core/langs/ucode/ir_variant.hpp"
#include "cpu/registermodel.hpp"
#include "cpu/statusbitmodel.hpp"
#include "debug/debugger.hpp"
#include "memory/hexdump/rawmemory.hpp"
#include "microobjectmodel.hpp"
#include "postmodel.hpp"
#include "project/architectures.hpp"
#include "project/levels.hpp"
#include "sim/debug/watchexpressionmodel.hpp"
#include "sim3/systems/traced_pep_isa3_system.hpp"
#include "sim3/systems/traced_pep_ma2_system.hpp"
#include "text/editor/editbase.hpp"
#include "text/editor/micro_line_numbers.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/symtab/symbolmodel.hpp"
#include "utils/opcodemodel.hpp"
#include "utils/strings.hpp"

namespace builtins {
class Registry;
}
class Pep_ISA : public QObject, public pepp::debug::Environment {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env CONSTANT)
  Q_PROPERTY(pepp::Architecture architecture READ architecture CONSTANT)
  Q_PROPERTY(pepp::Abstraction abstraction READ abstraction CONSTANT)
  Q_PROPERTY(int features READ features CONSTANT)
  Q_PROPERTY(QString objectCodeText READ objectCodeText WRITE setObjectCodeText NOTIFY objectCodeTextChanged);
  Q_PROPERTY(ARawMemory *memory READ memory CONSTANT)
  // Preserve the current address in the memory dump pane on tab-switch.
  Q_PROPERTY(quint16 currentAddress MEMBER _currentAddress NOTIFY currentAddressChanged)
  Q_PROPERTY(RegisterModel *registers MEMBER _registers CONSTANT)
  Q_PROPERTY(OpcodeModel *mnemonics READ mnemonics CONSTANT)
  Q_PROPERTY(FlagModel *flags MEMBER _flags CONSTANT)
  Q_PROPERTY(pepp::debug::BreakpointSet *breakpointModel READ breakpointModel CONSTANT)
  Q_PROPERTY(int allowedDebugging READ allowedDebugging NOTIFY allowedDebuggingChanged)
  // Step modes that are allowable for the current project type.
  Q_PROPERTY(int enabledSteps READ enabledSteps CONSTANT)
  // Step modes that should be active RIGHT NOW
  Q_PROPERTY(int allowedSteps READ allowedSteps NOTIFY allowedStepsChanged)
  Q_PROPERTY(QStringList saveAsOptions READ saveAsOptions CONSTANT)
  // Only changed externally
  Q_PROPERTY(QString charIn READ charIn WRITE setCharIn NOTIFY charInChanged)
  // Only changed internally.
  Q_PROPERTY(QString charOut READ charOut NOTIFY charOutChanged)
  Q_PROPERTY(bool isEmpty READ isEmpty)
  QML_UNCREATABLE("Can only be created through Project::")

public:
  enum class DebugVariables {
    A = 10,
    X,
    SP,
    PC,
    IS,
    OS,
  };
  Q_ENUM(DebugVariables);

  enum class UpdateType {
    Partial,
    Full,
  };
  explicit Pep_ISA(project::Environment env, QObject *parent = nullptr, bool initializeSystem = true);
  virtual project::Environment env() const;
  virtual pepp::Architecture architecture() const;
  virtual pepp::Abstraction abstraction() const;
  virtual int features() const;
  Q_INVOKABLE virtual QString delegatePath() const;
  ARawMemory *memory() const;
  OpcodeModel *mnemonics() const;
  QString objectCodeText() const;
  void setObjectCodeText(const QString &objectCodeText);
  Q_INVOKABLE static QStringListModel *modes() {
    static QStringListModel ret({"Welcome", "Help", "Editor", "Debugger"});
    QQmlEngine::setObjectOwnership(&ret, QQmlEngine::CppOwnership);
    return &ret;
  }
  // Actually utils::Abstraction, but QM passes it as an int.
  Q_INVOKABLE void set(int abstraction, QString value);
  Q_INVOKABLE pepp::debug::BreakpointSet *breakpointModel();
  Q_INVOKABLE virtual int allowedDebugging() const;
  Q_INVOKABLE virtual int enabledSteps() const;
  Q_INVOKABLE virtual int allowedSteps() const;
  Q_INVOKABLE QString charIn() const;
  Q_INVOKABLE void setCharIn(QString value);
  Q_INVOKABLE QString charOut() const;
  virtual bool isEmpty() const;

  virtual bool ignoreOS() const;
  virtual bool pcInOS() const;

  pepp::debug::types::TypeInfo *type_info() override;
  pepp::debug::types::TypeInfo const *type_info() const override;
  uint8_t read_mem_u8(uint32_t address) const override;
  uint16_t read_mem_u16(uint32_t address) const override;
  pepp::debug::Value evaluate_variable(QStringView name) const override;
  uint32_t cache_debug_variable_name(QStringView name) const override;
  pepp::debug::Value evaluate_debug_variable(uint32_t name) const override;

  virtual QStringList saveAsOptions() const { return {"pepo"}; }
  Q_INVOKABLE virtual QString defaultExtension() const { return "pepo"; }
  virtual QString contentsForExtension(const QString &ext) const;
public slots:
  virtual bool onLoadObject();
  bool onFormatObject();
  bool onExecute();
  virtual bool onDebuggingStart();
  bool onDebuggingContinue();
  bool onDebuggingPause();
  bool onDebuggingStop();
  bool onISARemoveAllBreakpoints();
  bool onISAStep();
  bool onISAStepOver();
  bool onISAStepInto();
  bool onISAStepOut();

  bool onClearCPU();
  bool onClearMemory();

  void onDeferredExecution(std::function<bool()> step);

signals:
  void objectCodeTextChanged();
  void currentAddressChanged();
  void allowedDebuggingChanged();
  void allowedStepsChanged();
  void charInChanged();
  void charOutChanged();
  // Called by onISARemoveAllBreakpoints so we can remove breakpoints from editors.
  void projectBreakpointsCleared();

  void message(QString message);
  void updateGUI(sim::api2::trace::FrameIterator from);
  void clearMessages();
  void deferredExecution(std::function<bool()> step);
  void overwriteEditors();

  // Propogated  C++ project model => C++ project => QML project wrapper => QML editor
  void markedClean();
  // Propogate  QML editor => QML project wrapper => C++ project => C++ project model
  void markDirty();

protected:
  void bindToSystem();
  bool _pendingPause = false;
  int _stepsSinceLastInteraction = 0;
  enum class State {
    Halted,
    NormalExec,
    DebugExec,
    DebugPaused,
  } _state = State::Halted;
  virtual void prepareSim();
  virtual void prepareGUIUpdate(sim::api2::trace::FrameIterator from);
  void updateMemPCSP() const;
  bool stepDepthHelper(qint16 offset);
  project::Environment _env;
  QString _charIn = {};
  QString _objectCodeText = {};
  QSharedPointer<sim::trace2::InfiniteBuffer> _tb = {};
  QSharedPointer<targets::isa::System> _system = {};
  QSharedPointer<ELFIO::elfio> _elf = {};
  // Use raw pointer to avoid double-free with parent'ed QObjects.
  SimulatorRawMemory *_memory = nullptr;
  RegisterModel *_registers = nullptr;
  FlagModel *_flags = nullptr;
  qint16 _currentAddress = 0;
  using Action = EditBase::Action;
  void updateBPAtAddress(quint32 address, Action action);
  QSharedPointer<pepp::debug::Debugger> _dbg{};
  QSharedPointer<builtins::Registry> _books = {};
  void loadCharIn();
  // TODO: at some point this type info needs to be extracted from the assembler + loader.
  pepp::debug::types::TypeInfo _typeInfo;
};

struct Error : public QObject {
  Q_OBJECT
  Q_PROPERTY(int line MEMBER line CONSTANT);
  Q_PROPERTY(QString message MEMBER error CONSTANT);

public:
  Error(int line, QString error, QObject *parent = nullptr);
  int line;
  QString error;
};

class Pep_ASMB final : public Pep_ISA {
  Q_OBJECT
  Q_PROPERTY(QString userAsmText READ userAsmText WRITE setUserAsmText NOTIFY userAsmTextChanged);
  Q_PROPERTY(QString userList READ userList NOTIFY listingChanged);
  Q_PROPERTY(QString osAsmText READ osAsmText WRITE setOSAsmText NOTIFY osAsmTextChanged);
  Q_PROPERTY(QString osList READ osList NOTIFY listingChanged);
  Q_PROPERTY(QList<Error *> assemblerErrors READ errors NOTIFY errorsChanged)
  Q_PROPERTY(StaticSymbolModel *staticSymbolModel READ staticSymbolModel CONSTANT)
  Q_PROPERTY(pepp::debug::WatchExpressionEditor *watchExpressions READ watchExpressions CONSTANT)
  Q_PROPERTY(pepp::debug::StackTracer *stackTracer READ stackTracer CONSTANT)
  Q_PROPERTY(ScopedLines2Addresses *lines2addr READ line2addr CONSTANT)
  Q_PROPERTY(bool ignoreOS READ ignoreOS CONSTANT)
  QML_UNCREATABLE("Can only be created through Project::")
  using Action = EditBase::Action;

public:
  explicit Pep_ASMB(project::Environment env, QObject *parent = nullptr);
  Q_INVOKABLE QString delegatePath() const override;
  // Actually utils::Abstraction, but QM passes it as an int.
  Q_INVOKABLE void set(int abstraction, QString value);
  Q_INVOKABLE QString userAsmText() const;
  Q_INVOKABLE void setUserAsmText(const QString &userAsmText);
  Q_INVOKABLE QString userList() const;
  Q_INVOKABLE QString osAsmText() const;
  Q_INVOKABLE void setOSAsmText(const QString &osAsmText);
  Q_INVOKABLE QString osList() const;
  Q_INVOKABLE const QList<Error *> errors() const;
  bool ignoreOS() const override;
  bool isEmpty() const override;
  Q_INVOKABLE StaticSymbolModel *staticSymbolModel() const;
  Q_INVOKABLE pepp::debug::WatchExpressionEditor *watchExpressions() const;
  Q_INVOKABLE pepp::debug::StackTracer *stackTracer() const;
  Q_INVOKABLE ScopedLines2Addresses *line2addr() const;
  int allowedDebugging() const override;
  QStringList saveAsOptions() const override { return {"pep", "pepl", "pepo"}; }
  QString defaultExtension() const override { return "pep"; }
  QString contentsForExtension(const QString &ext) const override;
public slots:
  bool onDebuggingStart() override;
  bool onAssemble(bool doLoad = false); // Wraps _onAssembler with an emit clearMessages().
  bool onAssembleThenLoad();
  bool onAssembleThenFormat();
  void onModifyUserSource(int line, Action action);
  void onModifyOSSource(int line, Action action);
  void onModifyUserList(int line, Action action);
  void onModifyOSList(int line, Action action);
  void onBPConditionChanged(quint16 address, bool conditional);
  void onClearEditorErrors();
signals:
  void userAsmTextChanged();
  void osAsmTextChanged();
  void listingChanged();
  void errorsChanged();
  void requestSourceBreakpoints();
  void clearListingBreakpoints();

  void switchTo(bool os);
  // Use inherited signal.
  // void updateGUI(sim::api2::trace::FrameIterator from);
  void modifyUserSource(int line, Action action);
  void modifyOSSource(int line, Action action);
  void modifyUserList(int line, Action action);
  void modifyOSList(int line, Action action);

protected:
  void prepareSim() override;
  void prepareGUIUpdate(sim::api2::trace::FrameIterator from) override;
  void updatePCLine();
  // Contains the real logic of onAssemble, but does not emit clearMessages.
  bool _onAssemble(bool doLoad = false);

  QString _userAsmText = {}, _osAsmText = {};
  QString _userList = {}, _osList = {};
  QList<QPair<int, QString>> _errors = {};
};

class Pep_MA : public QObject, public pepp::debug::Environment {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env CONSTANT)
  Q_PROPERTY(pepp::Architecture architecture READ architecture CONSTANT)
  Q_PROPERTY(pepp::Abstraction abstraction READ abstraction CONSTANT)
  Q_PROPERTY(int features READ features CONSTANT)
  Q_PROPERTY(QString lexerLanguage READ lexerLanguage CONSTANT)
  Q_PROPERTY(ARawMemory *memory READ memory CONSTANT)
  Q_PROPERTY(QString microcodeText READ microcodeText WRITE setMicrocodeText NOTIFY microcodeTextChanged);
  Q_PROPERTY(Microcode *microcode READ microcode NOTIFY microcodeChanged)
  Q_PROPERTY(pepp::LineNumbers *cycleNumbers READ line2addr NOTIFY microcodeChanged);
  Q_PROPERTY(QList<Error *> microassemblerErrors READ errors NOTIFY errorsChanged)
  // Preserve the current address in the memory dump pane on tab-switch.
  Q_PROPERTY(quint16 currentAddress READ currentAddress NOTIFY updateGUI)
  Q_PROPERTY(OpcodeModel *mnemonics READ mnemonics CONSTANT)
  Q_PROPERTY(PostModel *testResults READ testResults CONSTANT)
  Q_PROPERTY(pepp::debug::BreakpointSet *breakpointModel READ breakpointModel CONSTANT)
  Q_PROPERTY(int allowedDebugging READ allowedDebugging NOTIFY allowedDebuggingChanged)
  // Step modes that are allowable for the current project type.
  Q_PROPERTY(int enabledSteps READ enabledSteps CONSTANT)
  // Step modes that should be active RIGHT NOW
  Q_PROPERTY(int allowedSteps READ allowedSteps NOTIFY allowedStepsChanged)
  Q_PROPERTY(QStringList saveAsOptions READ saveAsOptions CONSTANT)
  Q_PROPERTY(int renderingType READ rendering_type CONSTANT)
  Q_PROPERTY(bool isEmpty READ isEmpty)
  QML_UNCREATABLE("Can only be created through Project::")
public:
  enum class UpdateType {
    Partial,
    Full,
  };
  explicit Pep_MA(project::Environment env, QObject *parent = nullptr);
  virtual project::Environment env() const;
  virtual pepp::Architecture architecture() const;
  virtual pepp::Abstraction abstraction() const;
  virtual int features() const;
  virtual QString lexerLanguage() const;
  Q_INVOKABLE virtual QString delegatePath() const;
  ARawMemory *memory() const;
  OpcodeModel *mnemonics() const;
  PostModel *testResults() const;
  QString microcodeText() const;
  void setMicrocodeText(const QString &microcodeText);
  Q_INVOKABLE static QStringListModel *modes() {
    static QStringListModel ret({"Welcome", "Help", "Editor", "Debugger"});
    QQmlEngine::setObjectOwnership(&ret, QQmlEngine::CppOwnership);
    return &ret;
  }
  Microcode *microcode() const;
  pepp::LineNumbers *line2addr() const;
  // Actually utils::Abstraction, but QM passes it as an int.
  Q_INVOKABLE void set(int abstraction, QString value);
  Q_INVOKABLE pepp::debug::BreakpointSet *breakpointModel();
  virtual bool isEmpty() const;
  int currentAddress() const;
  int allowedDebugging() const;
  int enabledSteps() const;
  int allowedSteps() const;
  virtual QStringList saveAsOptions() const { return {"pepcpu"}; }
  Q_INVOKABLE virtual QString defaultExtension() const { return "pepcpu"; }
  virtual QString contentsForExtension(const QString &ext) const;
  int rendering_type() const;
  QList<Error *> errors() const;

  // Can't place in signals/slots.
  using Action = EditBase::Action;

public slots:
  bool onMicroAssemble();
  bool onMicroAssembleThenFormat();

  bool onExecute();
  bool onDebuggingStart();
  bool onDebuggingContinue();
  bool onDebuggingPause();
  bool onDebuggingStop();
  bool onMARemoveAllBreakpoints();
  bool onMAStep();

  bool onClearCPU();
  bool onClearMemory();

  void onDeferredExecution(std::function<bool()> step);
  // Not tied to editorAction. This is the "receiving" side when the editor initiates a change.
  void onEditorAction(int line, Action action);

signals:
  void microcodeTextChanged();
  void currentAddressChanged();
  // Called by onISARemoveAllBreakpoints so we can remove breakpoints from editors.
  void projectBreakpointsCleared();
  void allowedStepsChanged();
  void allowedDebuggingChanged();
  void errorsChanged();
  void microcodeChanged();

  void message(QString message);
  void updateGUI(sim::api2::trace::FrameIterator from);
  void clearMessages();
  void deferredExecution(std::function<bool()> step);
  void overwriteEditors();
  void editorAction(int line, Action action);
  void requestEditorBreakpoints();

  // Propogated  C++ project model => C++ project => QML project wrapper => QML editor
  void markedClean();
  // Propogate  QML editor => QML project wrapper => C++ project => C++ project model
  void markDirty();

protected:
  void bindToSystem();
  bool _pendingPause = false;
  enum class State {
    Halted,
    NormalExec,
    DebugExec,
    DebugPaused,
  } _state = State::Halted;
  virtual void prepareSim();
  virtual void prepareGUIUpdate(sim::api2::trace::FrameIterator from);
  project::Environment _env;
  QSharedPointer<sim::trace2::InfiniteBuffer> _tb = {};
  QSharedPointer<targets::ma::System> _system = {};
  QString _microcodeText = {};
  // Use raw pointer to avoid double-free with parent'ed QObjects.
  SimulatorRawMemory *_memory = nullptr;
  PostModel *_testResults = nullptr;
  qint16 _currentAddress = 0;
  void updateBPAtAddress(quint32 address, Action action);
  void updatePC();
  QSharedPointer<pepp::debug::Debugger> _dbg{};
  QList<QPair<int, QString>> _errors = {};
  pepp::MicrocodeChoice _microcode = std::monostate{};
  pepp::TestChoice _testsPre = std::monostate{}, _testsPost = std::monostate{};
  pepp::Line2Address _line2addr;
  // TODO: at some point this type info needs to be extracted from the assembler + loader.
  pepp::debug::types::TypeInfo _typeInfo;

  // Dispatch between the handlers for each of the languages.
  // If override_source_text is true, _microcodeText will be updated on successful assembly.
  bool _microassemble(bool override_source_text);
  bool _microassemble8(bool override_source_text);
  bool _microassemble9_10_1(bool override_source_text);
  bool _microassemble9_10_2(bool override_source_text);
  // Update the number of tests rows and set the test names.
  void reloadPostTests();
  // Do NOT adjust the number of rows / the names of the tests. Only update value columns.
  void updatePostTestValues();

  // Environment interface
public:
  pepp::debug::types::TypeInfo *type_info() override;
  const pepp::debug::types::TypeInfo *type_info() const override;
  uint8_t read_mem_u8(uint32_t address) const override;
  uint16_t read_mem_u16(uint32_t address) const override;
  pepp::debug::Value evaluate_variable(QStringView name) const override;
  uint32_t cache_debug_variable_name(QStringView name) const override;
  pepp::debug::Value evaluate_debug_variable(uint32_t cache_index) const override;
};
