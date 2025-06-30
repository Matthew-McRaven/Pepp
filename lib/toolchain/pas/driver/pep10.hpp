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
#include "./pepp.hpp"
#include "enums/isa/pep10.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/operations/generic/include_macros.hpp"
#include "toolchain/symbol/table.hpp"

namespace pas::driver::pep10 {
static bool isDirectiveAddressed(ast::Node &node) {
  static const QSet<QString> dirs{"ALIGN", "ASCII", "WORD", "BYTE", "BLOCK"};
  if (node.get<ast::generic::Type>().value == ast::generic::Type::Directive && node.has<ast::generic::Directive>()) {
    auto directive = node.get<ast::generic::Directive>().value.toUpper();
    return dirs.contains(directive);
  }
  return false;
}
Q_NAMESPACE;
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

template <typename ParserTag> class TransformParse : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>, QSharedPointer<pas::driver::Target<Stage>> target) override {

    auto source = target->bodies[repr::Source::name];
    auto body = source.value<repr::Source>().value;
    auto parser = pas::driver::pepp::createParser<isa::Pep10, ParserTag>(false);
    auto parsed = parser(body, nullptr);
    int it = 0;
    auto children = pas::ast::children(*parsed.root);
    for (auto child : children) {
      child->template set<ast::generic::RootLocation>({.value = {it++}});
    }
    target->bodies[repr::Nodes::name] = QVariant::fromValue(repr::Nodes{.value = parsed.root});
    return !parsed.hadError;
  }
  Stage toStage() override { return Stage::IncludeMacros; }
};

template <typename ParserTag> class TransformIncludeMacros : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals> globals, QSharedPointer<pas::driver::Target<Stage>> target) override {
    auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;

    return pas::ops::generic::includeMacros(*root, pas::driver::pepp::createParser<isa::Pep10, ParserTag>(true),
                                            globals->macroRegistry, isDirectiveAddressed);
  }
  Stage toStage() override { return Stage::FlattenMacros; }
};

class TransformFlattenMacros : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>, QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

// Currently no-op
class TransformGroup : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>, QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};
class TransformRegisterExports : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>, QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};
class TransformAssignAddresses : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>, QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

class TransformWholeProgramSanity : public driver::Transform<Stage> {
public:
  bool isOS = false;
  bool ignoreUndefinedSymbols = false;
  bool operator()(QSharedPointer<Globals>, QSharedPointer<pas::driver::Target<Stage>> target) override;
  Stage toStage() override;
};

// No object representation (yet)
/*class TransformExportToObject : public driver::Transform<Stage> {
public:
  bool operator()(QSharedPointer<Globals>,
                  QSharedPointer<pas::driver::Target<Stage>>) override;
  Stage toStage() override { return Stage::IncludeMacros; };
};*/

struct Features {
  bool isOS = false;
  bool ignoreUndefinedSymbols = false;
};

struct TargetDefinition {
  Features enabledFeatures = {};
  QString body;
  Stage to = Stage::Start;
};

// Returns a single target's stages
template <typename ParserTag>
QPair<QSharedPointer<Target<Stage>>, QList<QSharedPointer<Transform<Stage>>>> stages(QString body, Features feats) {
  auto target = QSharedPointer<Target<Stage>>::create();
  target->stage = Stage::Start;
  target->kind = feats.isOS ? Target<Stage>::Kind::OS : Target<Stage>::Kind::User;
  target->symbolTable = QSharedPointer<symbol::Table>::create(2);
  target->bodies[repr::Source::name] = QVariant::fromValue(repr::Source{.value = body});

  QList<QSharedPointer<Transform<Stage>>> pipe;
  pipe.push_back(QSharedPointer<TransformParse<ParserTag>>::create());
  pipe.push_back(QSharedPointer<TransformIncludeMacros<ParserTag>>::create());
  pipe.push_back(QSharedPointer<TransformFlattenMacros>::create());
  pipe.push_back(QSharedPointer<TransformGroup>::create());
  pipe.push_back(QSharedPointer<TransformRegisterExports>::create());
  pipe.push_back(QSharedPointer<TransformAssignAddresses>::create());

  auto wps = QSharedPointer<TransformWholeProgramSanity>::create();
  wps->isOS = feats.isOS;
  wps->ignoreUndefinedSymbols = feats.ignoreUndefinedSymbols;
  pipe.push_back(wps);

  return {target, pipe};
}

template <typename ParserTag>
QSharedPointer<driver::Pipeline<Stage>> pipeline(QList<QPair<QString, Features>> targets,
                                                 QSharedPointer<macro::Registry> registry = nullptr) {

  auto ret = QSharedPointer<Pipeline<Stage>>::create();
  ret->globals = QSharedPointer<Globals>::create();
  if (registry) ret->globals->macroRegistry = registry;
  else ret->globals->macroRegistry = QSharedPointer<macro::Registry>::create();
  for (auto &[body, feats] : targets) ret->pipelines.push_back(stages<ParserTag>(body, feats));
  return ret;
}
} // namespace pas::driver::pep10
