#pragma once
#include "utils/constants.hpp"
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

// TODO: Expose values on AProject directly
struct Environment {
  utils::Architecture::Value arch;
  utils::Abstraction::Value level;
  Features features;
};
} // namespace project

// Dummy base class which provides functionality common to all projects.
class AProject : public QObject {
  Q_OBJECT
  Q_PROPERTY(project::Environment env READ env)
public:
  AProject(project::Environment env) : _env(env) {}
  project::Environment env() const { return _env; }
  virtual ~AProject() = default;
  // virtual void *memoryModel() = 0;
  // virtual void *cpuModel() = 0;

private:
  const project::Environment _env;
};

// Test class to handle ISA level of abstraction across all architectures.
class ISAProject final : public AProject {
  Q_OBJECT
  Q_PROPERTY(QString objectCodeText READ objectCodeText WRITE setObjectCodeText NOTIFY objectCodeTextChanged);

public:
  ISAProject(void *arch_info, project::Features features) : AProject({}) {}
  ~ISAProject() override = default;
  QString objectCodeText() const;
  void setObjectCodeText(const QString &objectCodeText);
signals:
  void objectCodeTextChanged();

private:
  QString _objectCodeText = {};
};

// Factory to ensure class invariants of project are maintained.
// Must be a singleton to call methods on it.
// Can't seem to call Q_INVOKABLE on an uncreatable type.
class Projects : public QObject {
  Q_OBJECT
public:
  Q_INVOKABLE ISAProject *isa(utils::Architecture::Value arch, project::Features features);
  Projects(QObject *parent = nullptr) : QObject(parent){};
};
