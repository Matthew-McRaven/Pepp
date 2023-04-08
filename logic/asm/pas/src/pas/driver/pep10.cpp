#include "pas/driver/pep10.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/flatten.hpp"
#include "pas/operations/generic/include_macros.hpp"
#include "pas/operations/generic/link_globals.hpp"
#include "pas/operations/pepp/assign_addr.hpp"
#include "pas/operations/pepp/register_system_calls.hpp"
#include "pas/operations/pepp/whole_program_sanity.hpp"

bool pas::driver::pep10::TransformParse::operator()(
    QSharedPointer<Globals>,
    QSharedPointer<pas::driver::Target<Stage>> target) {
  auto source = target->bodies[repr::Source::name];
  auto body = source.value<repr::Source>().value;
  auto parser = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false);
  auto parsed = parser(body, nullptr);
  target->bodies[repr::Nodes::name] =
      QVariant::fromValue(repr::Nodes{.value = parsed.root});
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
      *root, pas::driver::pepp::createParser<pas::isa::Pep10ISA>(true),
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
  pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*root);
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
  return pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
      *root, {.allowOSFeatures = isOS});
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
  target->symbolTable = QSharedPointer<symbol::Table>::create();
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
  pipe.push_back(wps);

  return {target, pipe};
}

QSharedPointer<pas::driver::Pipeline<pas::driver::pep10::Stage>>
pas::driver::pep10::pipeline(QList<QPair<QString, Features>> targets) {
  auto ret = QSharedPointer<Pipeline<Stage>>::create();
  ret->globals = QSharedPointer<Globals>::create();
  ret->globals->macroRegistry = QSharedPointer<macro::Registry>::create();
  for (auto &[body, feats] : targets)
    ret->pipelines.push_back(stages(body, feats));
  return ret;
}
