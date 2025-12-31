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
#include <QtQmlIntegration>
#include <qabstractitemmodel.h>
#include "sim3/systems/traced_pep_isa3_system.hpp"
#include "enums/constants.hpp"

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
