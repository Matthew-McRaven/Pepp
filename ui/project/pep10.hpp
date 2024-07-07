#pragma once

#include <QQmlEngine>
#include <QStringListModel>
#include <helpers/asmb.hpp>
#include <qabstractitemmodel.h>
#include <targets/pep10/isa3/system.hpp>
#include "./aproject.hpp"
#include "cpu/registermodel.hpp"
#include "cpu/statusbitmodel.hpp"
#include "memory/hexdump/rawmemory.hpp"
#include "text/editor/scintillaasmeditbase.hpp"
#include "utils/constants.hpp"
#include "utils/opcodemodel.hpp"

class Pep10_ISA : public QObject {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env CONSTANT)
  Q_PROPERTY(utils::Architecture architecture READ architecture CONSTANT)
  Q_PROPERTY(utils::Abstraction abstraction READ abstraction CONSTANT)
  Q_PROPERTY(QVariant delegate MEMBER _delegate NOTIFY delegateChanged)
  Q_PROPERTY(QString objectCodeText READ objectCodeText WRITE setObjectCodeText NOTIFY objectCodeTextChanged);
  Q_PROPERTY(ARawMemory *memory READ memory CONSTANT)
  // Preserve the current address in the memory dump pane on tab-switch.
  Q_PROPERTY(quint16 currentAddress MEMBER _currentAddress NOTIFY currentAddressChanged)
  Q_PROPERTY(RegisterModel *registers MEMBER _registers CONSTANT)
  Q_PROPERTY(OpcodeModel *mnemonics READ mnemonics CONSTANT)
  Q_PROPERTY(FlagModel *flags MEMBER _flags CONSTANT)
  Q_PROPERTY(int allowedDebugging READ allowedDebugging NOTIFY allowedDebuggingChanged)
  Q_PROPERTY(int allowedSteps READ allowedSteps NOTIFY allowedStepsChanged)
  // Only changed externally
  Q_PROPERTY(QString charIn READ charIn WRITE setCharIn)
  // Only changed internally.
  Q_PROPERTY(QString charOut READ charOut NOTIFY charOutChanged)
public:
  enum class UpdateType {
    Partial,
    Full,
  };
  explicit Pep10_ISA(QVariant delegate, QObject *parent = nullptr, bool initializeSystem = true);
  virtual project::Environment env() const;
  virtual utils::Architecture architecture() const;
  virtual utils::Abstraction abstraction() const;
  ARawMemory *memory() const;
  OpcodeModel *mnemonics() const;
  QString objectCodeText() const;
  void setObjectCodeText(const QString &objectCodeText);
  Q_INVOKABLE static QStringListModel *modes() {
    static QStringListModel ret({"Welcome", "Edit", "Debug", "Help"});
    QQmlEngine::setObjectOwnership(&ret, QQmlEngine::CppOwnership);
    return &ret;
  }
  // Actually utils::Abstraction, but QM passes it as an int.
  Q_INVOKABLE void set(int abstraction, QString value);
  Q_INVOKABLE virtual int allowedDebugging() const;
  Q_INVOKABLE virtual int allowedSteps() const;
  Q_INVOKABLE QString charIn() const;
  Q_INVOKABLE void setCharIn(QString value);
  Q_INVOKABLE QString charOut() const;
public slots:
  bool onSaveCurrent();
  bool onLoadObject();
  bool onExecute();
  bool onDebuggingStart();
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

signals:
  void objectCodeTextChanged();
  void delegateChanged();
  void currentAddressChanged();
  void allowedDebuggingChanged();
  void allowedStepsChanged();
  void charInChanged();
  void charOutChanged();

  void updateGUI(sim::api2::trace::FrameIterator from);

protected:
  void bindToSystem();
  enum class State {
    Halted,
    NormalExec,
    DebugExec,
    DebugPaused,
  } _state = State::Halted;
  virtual void prepareSim();
  virtual void prepareGUIUpdate(sim::api2::trace::FrameIterator from);
  QString _charIn = {};
  QString _objectCodeText = {};
  QVariant _delegate = {};
  QSharedPointer<sim::trace2::InfiniteBuffer> _tb = {};
  QSharedPointer<targets::pep10::isa::System> _system = {};
  QSharedPointer<ELFIO::elfio> _elf = {};
  // Use raw pointer to avoid double-free with parent'ed QObjects.
  SimulatorRawMemory *_memory = nullptr;
  RegisterModel *_registers = nullptr;
  FlagModel *_flags = nullptr;
  qint16 _currentAddress = 0;
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

class Pep10_ASMB final : public Pep10_ISA {
  Q_OBJECT
  Q_PROPERTY(QString userAsmText READ userAsmText WRITE setUserAsmText NOTIFY userAsmTextChanged);
  Q_PROPERTY(QString userList READ userList NOTIFY listingChanged);
  Q_PROPERTY(QList<Error *> userListAnnotations READ userListAnnotations NOTIFY listingChanged);
  Q_PROPERTY(QString osAsmText READ osAsmText WRITE setOSAsmText NOTIFY osAsmTextChanged);
  Q_PROPERTY(QString osList READ osList NOTIFY listingChanged);
  Q_PROPERTY(QList<Error *> osListAnnotations READ osListAnnotations NOTIFY listingChanged);
  Q_PROPERTY(QList<Error *> assemblerErrors READ errors NOTIFY errorsChanged)
  using Action = ScintillaAsmEditBase::Action;

public:
  explicit Pep10_ASMB(QVariant delegate, QObject *parent = nullptr);
  // Actually utils::Abstraction, but QM passes it as an int.
  Q_INVOKABLE void set(int abstraction, QString value);
  Q_INVOKABLE QString userAsmText() const;
  Q_INVOKABLE void setUserAsmText(const QString &userAsmText);
  Q_INVOKABLE QString userList() const;
  Q_INVOKABLE const QList<Error *> userListAnnotations() const;
  Q_INVOKABLE QString osAsmText() const;
  Q_INVOKABLE void setOSAsmText(const QString &osAsmText);
  Q_INVOKABLE QString osList() const;
  Q_INVOKABLE const QList<Error *> osListAnnotations() const;
  Q_INVOKABLE const QList<Error *> errors() const;
  project::Environment env() const override;
  utils::Architecture architecture() const override;
  utils::Abstraction abstraction() const override;
  int allowedDebugging() const override;
public slots:
  bool onAssemble(bool doLoad = false);
  bool onAssembleThenFormat();
  void onModifyUserSource(int line, Action action);
  void onModifyOSSource(int line, Action action);
  void onModifyUserList(int line, Action action);
  void onModifyOSList(int line, Action action);
signals:
  void userAsmTextChanged();
  void osAsmTextChanged();
  void listingChanged();
  void errorsChanged();
  void requestSourceBreakpoints();
  void clearListingBreakpoints();

  void updateGUI(sim::api2::trace::FrameIterator from);
  void message(QString message);
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
  QList<QPair<int, QString>> _errors = {}, _userListAnnotations = {}, _osListAnnotations = {};
  helpers::AsmHelper::Lines2Addresses _userLines2Address = {}, _osLines2Address = {};
  void updateBPAtAddress(quint32 address, Action action);
  sim::api2::trace::ValueFilter<quint8> *_breakpoints = nullptr;
};
