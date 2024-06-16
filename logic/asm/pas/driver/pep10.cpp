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
#include "asm/pas/operations/generic/combine.hpp"
#include "asm/pas/operations/generic/flatten.hpp"
#include "asm/pas/operations/generic/group.hpp"
#include "asm/pas/operations/generic/link_globals.hpp"
#include "asm/pas/operations/pepp/addressable.hpp"
#include "asm/pas/operations/pepp/assign_addr.hpp"
#include "asm/pas/operations/pepp/register_system_calls.hpp"
#include "asm/pas/operations/pepp/whole_program_sanity.hpp"

bool pas::driver::pep10::TransformFlattenMacros::operator()(QSharedPointer<Globals>,
                                                            QSharedPointer<pas::driver::Target<Stage>> target) {
  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::generic::flattenMacros(*root);
  return true;
}

pas::driver::pep10::Stage pas::driver::pep10::TransformFlattenMacros::toStage() { return Stage::GroupNodes; }

bool pas::driver::pep10::TransformGroup::operator()(QSharedPointer<Globals>,
                                                    QSharedPointer<pas::driver::Target<Stage>> target) {
  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::generic::groupSections(*root, pas::ops::pepp::isAddressable<isa::Pep10>);
  return true;
}

pas::driver::pep10::Stage pas::driver::pep10::TransformGroup::toStage() { return Stage::RegisterExports; }

bool pas::driver::pep10::TransformRegisterExports::operator()(QSharedPointer<Globals> globals,
                                                              QSharedPointer<pas::driver::Target<Stage>> target) {

  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::generic::linkGlobals(*root, globals, {u"EXPORT"_qs});
  return pas::ops::pepp::registerSystemCalls(*root, globals->macroRegistry);
}

pas::driver::pep10::Stage pas::driver::pep10::TransformRegisterExports::toStage() { return Stage::AssignAddresses; }

bool pas::driver::pep10::TransformAssignAddresses::operator()(QSharedPointer<Globals>,
                                                              QSharedPointer<pas::driver::Target<Stage>> target) {

  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::pepp::assignAddresses<isa::Pep10>(*root);
  pas::ops::generic::concatSectionAddresses(*root);
  return true;
}

pas::driver::pep10::Stage pas::driver::pep10::TransformAssignAddresses::toStage() { return Stage::WholeProgramSanity; }

bool pas::driver::pep10::TransformWholeProgramSanity::operator()(QSharedPointer<Globals>,
                                                                 QSharedPointer<pas::driver::Target<Stage>> target) {
  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  // TODO: tie class variable to OS features.
  return pas::ops::pepp::checkWholeProgramSanity<isa::Pep10>(
      *root, {.allowOSFeatures = isOS, .ignoreUndefinedSymbols = ignoreUndefinedSymbols});
}

pas::driver::pep10::Stage pas::driver::pep10::TransformWholeProgramSanity::toStage() { return Stage::End; }
