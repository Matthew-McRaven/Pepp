/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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
#include "./common.hpp"
#include "asm/pas/pas_globals.hpp"

namespace pas::driver::pep10 {
Q_NAMESPACE_EXPORT(PAS_EXPORT)
enum class Stage {
  Start,
  Parse,
  IncludeMacros,
  FlattenMacros,
  GroupNodes,
  RegisterExports,
  AssignAddresses,
  WholeProgramSanity,
  ExportToObject,
  End
};
Q_ENUM_NS(Stage);

class PAS_EXPORT TransformParse : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

class PAS_EXPORT TransformIncludeMacros : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

class PAS_EXPORT TransformFlattenMacros : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

// Currently no-op
class PAS_EXPORT TransformGroup : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};
class PAS_EXPORT TransformRegisterExports : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};
class PAS_EXPORT TransformAssignAddresses : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

class PAS_EXPORT TransformWholeProgramSanity : public driver::Transform<Stage> {
public:
  bool isOS = false;
  bool ignoreUndefinedSymbols = false;
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

// No object representation (yet)
/*class TransformExportToObject : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>>) override;
  Stage toStage() override { return Stage::IncludeMacros; };
};*/

struct PAS_EXPORT Features {
  bool isOS = false;
  bool ignoreUndefinedSymbols = false;
};

struct PAS_EXPORT TargetDefinition {
  Features enabledFeatures = {};
  QString body;
  Stage to = Stage::Start;
};
// Returns a single target's stages
QPair<QSharedPointer<Target<Stage>>, QList<QSharedPointer<Transform<Stage>>>>
PAS_EXPORT stages(QString body, Features feats);

QSharedPointer<driver::Pipeline<Stage>>
PAS_EXPORT pipeline(QList<QPair<QString, Features>> targets,
         QSharedPointer<macro::Registry> registry = nullptr);
} // namespace pas::driver::pep10
