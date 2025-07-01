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

#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/operations/generic/combine.hpp"
#include "toolchain/pas/operations/generic/group.hpp"
#include "toolchain/pas/operations/generic/link_globals.hpp"
#include "toolchain/pas/operations/pepp/addressable.hpp"
#include "toolchain/pas/operations/pepp/assign_addr.hpp"
#include "toolchain/pas/operations/pepp/register_system_calls.hpp"
#include "toolchain/pas/operations/pepp/whole_program_sanity.hpp"

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
  pas::ops::generic::linkGlobals(*root, globals, {"EXPORT"});
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
  int it = 0;
  auto sections = pas::ast::children(*root);
  for (auto &section : sections) {
    auto children = pas::ast::children(*section);
    for (auto &child : children) {
      child->set(ast::generic::ListingLocation{it});
      if (child->has<ast::generic::Hide>() &&
          child->get<ast::generic::Hide>().value.object != ast::generic::Hide::In::Object::Emit) {
        it += 1;
      } else if (auto size = ops::pepp::implicitSize<isa::Pep10>(*child); size <= 3) it += 1;
      else it += (size + 2) / 3;
    }
  }
  // TODO: tie class variable to OS features.
  return pas::ops::pepp::checkWholeProgramSanity<isa::Pep10>(
      *root, {.allowOSFeatures = isOS, .ignoreUndefinedSymbols = ignoreUndefinedSymbols});
}

pas::driver::pep10::Stage pas::driver::pep10::TransformWholeProgramSanity::toStage() { return Stage::End; }
