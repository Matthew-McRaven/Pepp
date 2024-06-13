#pragma once
#include <QQmlEngine>
#include <QStringListModel>
#include <deque>
#include <qabstractitemmodel.h>
#include <targets/pep10/isa3/system.hpp>
#include "cpu/registermodel.hpp"
#include "cpu/statusbitmodel.hpp"
#include "memory/hexdump/rawmemory.hpp"
#include "utils/constants.hpp"
#include "utils/opcodemodel.hpp"

namespace sim {
namespace trace2 {
class InfiniteBuffer;
}
} // namespace sim
namespace project {
// Additional options requested for a project.
// A particular (arch, level) tuple may only support a subset of features.
// TODO: Wrap in a Q_OBJECT to expose to QML.
enum class Features : int {
  None = 0,
  OneByte,
  TwoByte,
  NoOS,
};

class DebugEnableFlags : public QObject {
  Q_OBJECT
public:
  explicit DebugEnableFlags(QObject *parent = nullptr);
  enum Value {
    Start = 1,
    Continue = 2,
    Pause = 4,
    Stop = 8,
    LoadObject = 16,
    Execute = 32,
  };
  Q_ENUM(Value);
};

class StepEnableFlags : public QObject {
  Q_OBJECT
public:
  explicit StepEnableFlags(QObject *parent = nullptr);
  enum Value {
    Step = 1,
    StepOver = 2,
    StepInto = 4,
    StepOut = 8,
    StepBack = 16,
    StepBackOver = 32,
    StepBackInto = 64,
    StepBackOut = 128,
  };
  Q_ENUM(Value);
};

// TODO: Expose values on AProject directly
struct Environment {
  utils::Architecture arch;
  utils::Abstraction level;
  Features features;
};
} // namespace project

// Dummy base class which provides functionality common to all projects.
class AProject : public QObject {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env)
public:
  explicit AProject(project::Environment env) : _env(env) {}
  project::Environment env() const { return _env; }
  virtual ~AProject() = default;
  // virtual void *memoryModel() = 0;
  // virtual void *cpuModel() = 0;

private:
  const project::Environment _env;
};

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
  explicit Pep10_ISA(QVariant delegate, QObject *parent = nullptr);
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
  Q_INVOKABLE int allowedDebugging() const;
  Q_INVOKABLE int allowedSteps() const;
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
  enum class State {
    Halted,
    NormalExec,
    DebugExec,
    DebugPaused,
  } _state = State::Halted;
  void prepareSim();
  void prepareGUIUpdate(sim::api2::trace::FrameIterator from);
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

class Pep10_ASMB final : public Pep10_ISA {
  Q_OBJECT
  Q_PROPERTY(QString userAsmText READ userAsmText WRITE setUserAsmText NOTIFY userAsmTextChanged);

public:
  explicit Pep10_ASMB(QVariant delegate, QObject *parent = nullptr);
  // Actually utils::Abstraction, but QM passes it as an int.
  Q_INVOKABLE void set(int abstraction, QString value);
  Q_INVOKABLE QString userAsmText() const;
  Q_INVOKABLE void setUserAsmText(const QString &userAsmText);
  project::Environment env() const override;
  utils::Architecture architecture() const override;
  utils::Abstraction abstraction() const override;
signals:
  void userAsmTextChanged();
  void updateGUI(sim::api2::trace::FrameIterator from);

protected:
  void prepareSim();
  void prepareGUIUpdate(sim::api2::trace::FrameIterator from);
  QString _userAsmText = {};
};

// Factory to ensure class invariants of project are maintained.
// Must be a singleton to call methods on it.
// Can't seem to call Q_INVOKABLE on an uncreatable type.
class ProjectModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ _rowCount NOTIFY rowCountChanged)
public:
  enum class Roles {
    ProjectRole = Qt::UserRole + 1,
  };
  Q_ENUM(Roles);
  // Q_INVOKABLE ISAProject *isa(utils::Architecture::Value arch, project::Features features);
  explicit ProjectModel(QObject *parent = nullptr) : QAbstractListModel(parent){};
  // Helper to expose rowCount as a property to QML.
  int _rowCount() const { return rowCount({}); }
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  Q_INVOKABLE Pep10_ISA *pep10ISA(QVariant delegate);
  Q_INVOKABLE Pep10_ASMB *pep10ASMB(QVariant delegate);
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                int destinationChild) override;
  QHash<int, QByteArray> roleNames() const override;
signals:
  void rowCountChanged(int);

private:
  std::deque<std::unique_ptr<QObject>> _projects = {};
};
