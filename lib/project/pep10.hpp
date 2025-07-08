#pragma once

#include <QQmlEngine>
#include <QStringListModel>
#include <qabstractitemmodel.h>
#include "aproject.hpp"
#include "cpu/registermodel.hpp"
#include "cpu/statusbitmodel.hpp"
#include "debug/debugger.hpp"
#include "enums/constants.hpp"
#include "memory/hexdump/rawmemory.hpp"
#include "sim/debug/watchexpressionmodel.hpp"
#include "targets/isa3/system.hpp"
#include "text/editor/scintillaasmeditbase.hpp"
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
  Q_PROPERTY(QString objectCodeText READ objectCodeText WRITE setObjectCodeText NOTIFY objectCodeTextChanged);
  Q_PROPERTY(ARawMemory *memory READ memory CONSTANT)
  // Preserve the current address in the memory dump pane on tab-switch.
  Q_PROPERTY(quint16 currentAddress MEMBER _currentAddress NOTIFY currentAddressChanged)
  Q_PROPERTY(RegisterModel *registers MEMBER _registers CONSTANT)
  Q_PROPERTY(OpcodeModel *mnemonics READ mnemonics CONSTANT)
  Q_PROPERTY(FlagModel *flags MEMBER _flags CONSTANT)
  Q_PROPERTY(pepp::debug::BreakpointSet *breakpointModel READ breakpointModel CONSTANT)
  Q_PROPERTY(int allowedDebugging READ allowedDebugging NOTIFY allowedDebuggingChanged)
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
  Q_INVOKABLE virtual int allowedSteps() const;
  Q_INVOKABLE QString charIn() const;
  Q_INVOKABLE void setCharIn(QString value);
  Q_INVOKABLE QString charOut() const;
  virtual bool isEmpty() const;

  pepp::debug::types::RuntimeTypeInfo *type_info() override;
  pepp::debug::types::RuntimeTypeInfo const *type_info() const override;
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

  void message(QString message);
  void updateGUI(sim::api2::trace::FrameIterator from);
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
  using Action = ScintillaAsmEditBase::Action;
  void updateBPAtAddress(quint32 address, Action action);
  QSharedPointer<pepp::debug::Debugger> _dbg{};
  QSharedPointer<builtins::Registry> _books = {};
  void loadCharIn();
  // TODO: at some point this type info needs to be extracted from the assembler + loader.
  pepp::debug::types::RuntimeTypeInfo _info;
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
  Q_PROPERTY(ScopedLines2Addresses *lines2addr READ line2addr CONSTANT)
  QML_UNCREATABLE("Can only be created through Project::")
  using Action = ScintillaAsmEditBase::Action;

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
  bool isEmpty() const override;
  Q_INVOKABLE StaticSymbolModel *staticSymbolModel() const;
  Q_INVOKABLE pepp::debug::WatchExpressionEditor *watchExpressions() const;
  Q_INVOKABLE ScopedLines2Addresses *line2addr() const;
  int allowedDebugging() const override;
  QStringList saveAsOptions() const override { return {"pep", "pepl", "pepo"}; }
  QString defaultExtension() const override { return "pep"; }
  QString contentsForExtension(const QString &ext) const override;
public slots:
  bool onDebuggingStart() override;
  bool onAssemble(bool doLoad = false);
  bool onAssembleThenLoad();
  bool onAssembleThenFormat();
  void onModifyUserSource(int line, Action action);
  void onModifyOSSource(int line, Action action);
  void onModifyUserList(int line, Action action);
  void onModifyOSList(int line, Action action);
  void onBPConditionChanged(quint16 address, bool conditional);
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

  QString _userAsmText = {}, _osAsmText = {};
  QString _userList = {}, _osList = {};
  QList<QPair<int, QString>> _errors = {};
};
