#pragma once
#include <QQmlEngine>
#include <QStringListModel>
#include <QtQmlIntegration>
#include <qabstractitemmodel.h>
#include "enums/constants.hpp"
#include "targets/isa3/system.hpp"

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
  QML_NAMED_ELEMENT(DebugEnableFlags)
  QML_UNCREATABLE("Error: Only enums")

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
  QML_NAMED_ELEMENT(StepEnableFlags)
  QML_UNCREATABLE("Error: Only enums")

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
  pepp::Architecture arch;
  pepp::Abstraction level;
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
