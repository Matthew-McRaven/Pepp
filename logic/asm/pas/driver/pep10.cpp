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

#include "asm/pas/driver/pep10.hpp"
#include "isa/pep10.hpp"
#include "asm/pas/driver/pepp.hpp"
#include "asm/pas/operations/generic/combine.hpp"
#include "asm/pas/operations/generic/flatten.hpp"
#include "asm/pas/operations/generic/group.hpp"
#include "asm/pas/operations/generic/include_macros.hpp"
#include "asm/pas/operations/generic/link_globals.hpp"
#include "asm/pas/operations/pepp/addressable.hpp"
#include "asm/pas/operations/pepp/assign_addr.hpp"
#include "asm/pas/operations/pepp/register_system_calls.hpp"
#include "asm/pas/operations/pepp/whole_program_sanity.hpp"

bool pas::driver::pep10::TransformParse::operator()(
    QSharedPointer<Globals>,
    QSharedPointer<pas::driver::Target<Stage>> target) {
  auto source = target->bodies[repr::Source::name];
  auto body = source.value<repr::Source>().value;
  auto parser = pas::driver::pepp::createParser<isa::Pep10>(false);
  auto parsed = parser(body, nullptr);
  target->bodies[repr::Nodes::name] =
      QVariant::fromValue(repr::Nodes{.value = parsed.root});
  // FIX: Remove when CI bug is fixed.
  if (parsed.hadError) {
    qWarning() << parsed.errors;
  }
  return !parsed.hadError;
}

pas::driver::pep10::Stage pas::driver::pep10::TransformParse::toStage() {
  return Stage::IncludeMacros;
}

bool pas::driver::pep10::TransformIncludeMacros::operator()(
    QSharedPointer<Globals> globals,
    QSharedPointer<pas::driver::Target<Stage>> target) {
  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  return pas::ops::generic::includeMacros(
      *root, pas::driver::pepp::createParser<isa::Pep10>(true),
      globals->macroRegistry);
};

pas::driver::pep10::Stage
pas::driver::pep10::TransformIncludeMacros::toStage() {
  return Stage::FlattenMacros;
}

bool pas::driver::pep10::TransformFlattenMacros::operator()(
    QSharedPointer<Globals>,
    QSharedPointer<pas::driver::Target<Stage>> target) {
  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::generic::flattenMacros(*root);
  return true;
}

pas::driver::pep10::Stage
pas::driver::pep10::TransformFlattenMacros::toStage() {
  return Stage::GroupNodes;
}

bool pas::driver::pep10::TransformGroup::operator()(
    QSharedPointer<Globals>,
    QSharedPointer<pas::driver::Target<Stage>> target) {
  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::generic::groupSections(*root,
                                   pas::ops::pepp::isAddressable<isa::Pep10>);
  return true;
}

pas::driver::pep10::Stage pas::driver::pep10::TransformGroup::toStage() {
  return Stage::RegisterExports;
}

bool pas::driver::pep10::TransformRegisterExports::operator()(
    QSharedPointer<Globals> globals,
    QSharedPointer<pas::driver::Target<Stage>> target) {

  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::generic::linkGlobals(*root, globals, {u"EXPORT"_qs});
  return pas::ops::pepp::registerSystemCalls(*root, globals->macroRegistry);
}

pas::driver::pep10::Stage
pas::driver::pep10::TransformRegisterExports::toStage() {
  return Stage::AssignAddresses;
}

bool pas::driver::pep10::TransformAssignAddresses::operator()(
    QSharedPointer<Globals>,
    QSharedPointer<pas::driver::Target<Stage>> target) {

  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::pepp::assignAddresses<isa::Pep10>(*root);
  pas::ops::generic::concatSectionAddresses(*root);
  return true;
}

pas::driver::pep10::Stage
pas::driver::pep10::TransformAssignAddresses::toStage() {
  return Stage::WholeProgramSanity;
}

bool pas::driver::pep10::TransformWholeProgramSanity::operator()(
    QSharedPointer<Globals>,
    QSharedPointer<pas::driver::Target<Stage>> target) {
  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  // TODO: tie class variable to OS features.
  return pas::ops::pepp::checkWholeProgramSanity<isa::Pep10>(
      *root, {.allowOSFeatures = isOS,
              .ignoreUndefinedSymbols = ignoreUndefinedSymbols});
}

pas::driver::pep10::Stage
pas::driver::pep10::TransformWholeProgramSanity::toStage() {
  return Stage::End;
}

QPair<QSharedPointer<pas::driver::Target<pas::driver::pep10::Stage>>,
      QList<QSharedPointer<pas::driver::Transform<pas::driver::pep10::Stage>>>>
pas::driver::pep10::stages(QString body, Features feats) {
  auto target = QSharedPointer<Target<Stage>>::create();
  target->stage = Stage::Start;
  target->kind =
      feats.isOS ? Target<Stage>::Kind::OS : Target<Stage>::Kind::User;
  target->symbolTable = QSharedPointer<symbol::Table>::create(2);
  target->bodies[repr::Source::name] =
      QVariant::fromValue(repr::Source{.value = body});

  QList<QSharedPointer<Transform<Stage>>> pipe;
  pipe.push_back(QSharedPointer<TransformParse>::create());
  pipe.push_back(QSharedPointer<TransformIncludeMacros>::create());
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

QSharedPointer<pas::driver::Pipeline<pas::driver::pep10::Stage>>
pas::driver::pep10::pipeline(QList<QPair<QString, Features>> targets,
                             QSharedPointer<macro::Registry> registry) {
  auto ret = QSharedPointer<Pipeline<Stage>>::create();
  ret->globals = QSharedPointer<Globals>::create();
  if (registry)
    ret->globals->macroRegistry = registry;
  else
    ret->globals->macroRegistry = QSharedPointer<macro::Registry>::create();
  for (auto &[body, feats] : targets)
    ret->pipelines.push_back(stages(body, feats));
  return ret;
}
