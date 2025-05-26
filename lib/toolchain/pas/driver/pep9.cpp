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

#include "toolchain/pas/driver/pep9.hpp"
#include "toolchain/pas/operations/generic/combine.hpp"
#include "toolchain/pas/operations/generic/flatten.hpp"
#include "toolchain/pas/operations/generic/link_globals.hpp"
#include "toolchain/pas/operations/pepp/assign_addr.hpp"
#include "toolchain/pas/operations/pepp/whole_program_sanity.hpp"


bool pas::driver::pep9::TransformRegisterExports::operator()(QSharedPointer<Globals> globals,
                                                             QSharedPointer<pas::driver::Target<Stage>> target) {

  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  auto st = root->get<pas::ast::generic::SymbolTable>().value;
  if (auto charOut = st->get("charOut"); charOut) globals->add(*charOut);
  if (auto charIn = st->get("charIn"); charIn) globals->add(*charIn);
  // Needed to make user program point to correct charIn/charOut.
  pas::ops::generic::linkGlobals(*root, globals, {});
  return true;
}

pas::driver::pep9::Stage pas::driver::pep9::TransformRegisterExports::toStage() { return Stage::AssignAddresses; }

bool pas::driver::pep9::TransformAssignAddresses::operator()(QSharedPointer<Globals>,
                                                             QSharedPointer<pas::driver::Target<Stage>> target) {

  auto root = target->bodies[repr::Nodes::name].value<repr::Nodes>().value;
  pas::ops::pepp::assignAddresses<isa::Pep9>(*root);
  pas::ops::generic::concatSectionAddresses(*root);
  return true;
}

pas::driver::pep9::Stage pas::driver::pep9::TransformAssignAddresses::toStage() { return Stage::WholeProgramSanity; }

bool pas::driver::pep9::TransformWholeProgramSanity::operator()(QSharedPointer<Globals>,
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
      } else if (auto size = ops::pepp::implicitSize<isa::Pep9>(*child); size <= 3) it += 1;
      else it += (size + 2) / 3;
    }
  }
  // TODO: tie class variable to OS features.
  return pas::ops::pepp::checkWholeProgramSanity<isa::Pep9>(
      *root, {.allowOSFeatures = isOS, .ignoreUndefinedSymbols = ignoreUndefinedSymbols});
}

pas::driver::pep9::Stage pas::driver::pep9::TransformWholeProgramSanity::toStage() { return Stage::End; }
