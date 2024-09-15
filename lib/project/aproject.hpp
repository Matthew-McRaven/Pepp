#pragma once
#include <QQmlEngine>
#include <QStringListModel>
#include <deque>
#include <qabstractitemmodel.h>
#include <targets/pep10/isa3/system.hpp>
#include "builtins/constants.hpp"
#include "cpu/registermodel.hpp"
#include "cpu/statusbitmodel.hpp"
#include "memory/hexdump/rawmemory.hpp"
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
    Continue = 256,
  };
  Q_ENUM(Value);
};

// TODO: Expose values on AProject directly
struct Environment {
  builtins::Architecture arch;
  builtins::Abstraction level;
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

class Pep10_ISA;
class Pep10_ASMB;
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
  Q_INVOKABLE Pep10_ASMB *pep10ASMB(QVariant delegate, builtins::Abstraction abstraction);
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                int destinationChild) override;
  QHash<int, QByteArray> roleNames() const override;
signals:
  void rowCountChanged(int);

private:
  std::deque<std::unique_ptr<QObject>> _projects = {};
};
