/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "toolchain/pas/operations/pepp/whole_program_sanity.hpp"
#include <catch.hpp>
#include "../../../../../lib/core/isa/pep/pep10.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/driver/pepp.hpp"
#include "toolchain/pas/errors.hpp"
#include "toolchain/pas/operations/generic/group.hpp"
#include "toolchain/pas/operations/pepp/addressable.hpp"
#include "toolchain/pas/operations/pepp/assign_addr.hpp"

namespace E = pas::errors::pepp;
namespace {
void smoke(QString source, QStringList errors, bool useDriver, bool useOSFeats) {
  QSharedPointer<pas::ast::Node> root;
  if (useDriver) {
    auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(source, {.isOS = useOSFeats});
    auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
    pipelines.pipelines.push_back(pipeline);
    pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
    pipelines.globals->macroRegistry = QSharedPointer<macro::Registry>::create();
    CHECK(pipelines.assemble(pas::driver::pep10::Stage::WholeProgramSanity) == (errors.size() == 0));
    CHECK(pipelines.pipelines[0].first->stage ==
          (errors.size() == 0 ? pas::driver::pep10::Stage::End : pas::driver::pep10::Stage::WholeProgramSanity));
    REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
    root = pipelines.pipelines[0].first->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  } else {
    auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
    auto res = parseRoot(source, nullptr);
    REQUIRE_FALSE(res.hadError);
    pas::ops::generic::groupSections(*res.root, pas::ops::pepp::isAddressable<isa::Pep10>);
    pas::ops::pepp::assignAddresses<isa::Pep10>(*res.root);
    root = res.root;
    CHECK(pas::ops::pepp::checkWholeProgramSanity<isa::Pep10>(*root, {.allowOSFeatures = useOSFeats}) ==
          (errors.size() == 0));
  }
  auto actualErrors = pas::ops::generic::collectErrors(*root);
  CHECK(actualErrors.size() == errors.size());
  for (int it = 0; it < actualErrors.size(); it++)
    CHECK(actualErrors[it].second.message.toStdString() == errors[it].toStdString());
}
} // namespace

TEST_CASE("Whole Program Sanity", "[scope:asm][kind:unit][arch:pep10]") {
  auto [name, source, errors, useDriver, useOSFeats] = GENERATE(table<std::string, QString, QStringList, bool, bool>({
      {"noBurn: visitor", {".BURN 0xFFFF\n.BLOCK 1"}, {".BURN is not a valid directive."}, false, false},
      {"noBurn: driver", {".BURN 0xFFFF\n.BLOCK 1"}, {".BURN is not a valid directive."}, true, false},
      {"size0xFFFF: visitor", {".BLOCK 0xFFFF"}, {}, false, false},
      {"size0xFFFF: driver", {".BLOCK 0xFFFF"}, {}, true, false},
      {"size0x10000: visitor", {".BLOCK 0xFFFF\n.block 2"}, {"Object code must fit within 65536 bytes."}, false, false},
      {"size0x10000: driver", {".BLOCK 0xFFFF\n.block 2"}, {"Object code must fit within 65536 bytes."}, true, false},

      {".IMPORT in user: visitor", ".IMPORT s\ns:.block 1\n", {E::illegalInUser.arg(".IMPORT")}, false, false},
      {".IMPORT in user: driver", ".IMPORT s\ns:.block 1\n", {E::illegalInUser.arg(".IMPORT")}, true, false},
      {".IMPORT in OS: visitor", ".IMPORT s\ns:.block 1\n", {}, false, true},
      {".IMPORT in OS: driver", ".IMPORT s\ns:.block 1\n", {}, true, true},

      {".EXPORT in user: visitor", ".EXPORT s\ns:.block 1\n", {E::illegalInUser.arg(".EXPORT")}, false, false},
      {".EXPORT in user: driver", ".EXPORT s\ns:.block 1\n", {E::illegalInUser.arg(".EXPORT")}, true, false},
      {".EXPORT in OS: visitor", ".EXPORT s\ns:.block 1\n", {}, false, true},
      {".EXPORT in OS: driver", ".EXPORT s\ns:.block 1\n", {}, true, true},

      {".SCALL in user: visitor", ".SCALL s\ns:.block 1\n", {E::illegalInUser.arg(".SCALL")}, false, false},
      {".SCALL in user: driver", ".SCALL s\ns:.block 1\n", {E::illegalInUser.arg(".SCALL")}, true, false},
      {".SCALL in OS: visitor", ".SCALL s\ns:.block 1\n", {}, false, true},
      {".SCALL in OS: driver", ".SCALL s\ns:.block 1\n", {}, true, true},

      {".INPUT in user: visitor", ".INPUT s\ns:.block 1\n", {E::illegalInUser.arg(".INPUT")}, false, false},
      {".INPUT in user: driver", ".INPUT s\ns:.block 1\n", {E::illegalInUser.arg(".INPUT")}, true, false},
      {".INPUT in OS: visitor", ".INPUT s\ns:.block 1\n", {}, false, true},
      {".INPUT in OS: driver", ".INPUT s\ns:.block 1\n", {}, true, true},

      {".OUTPUT in user: visitor", ".OUTPUT s\ns:.block 1\n", {E::illegalInUser.arg(".OUTPUT")}, false, false},
      {".OUTPUT in user: driver", ".OUTPUT s\ns:.block 1\n", {E::illegalInUser.arg(".OUTPUT")}, true, false},
      {".OUTPUT in OS: visitor", ".OUTPUT s\ns:.block 1\n", {}, false, true},
      {".OUTPUT in OS: driver", ".OUTPUT s\ns:.block 1\n", {}, true, true},

      {"no END: visitor", ".BLOCK 0xFFFF\n .END", {".END is not a valid directive."}, false, false},
      {"no END: driver", ".BLOCK 0xFFFF\n .END", {".END is not a valid directive."}, true, false},
      {"no undefined args: visitor", "LDWA s,i", {"Undefined symbol s."}, false, false},
      {"no undefined args: driver", "LDWA s,i", {"Undefined symbol s."}, true, false},
      {"no multiply defined symbols: visitor",
       "s:.BLOCK 2\ns:.block 2",
       {"Multiply defined symbol s.", "Multiply defined symbol s."},
       false,
       false},
      {"no multiply defined symbols: driver",
       "s:.BLOCK 2\ns:.block 2",
       {"Multiply defined symbol s.", "Multiply defined symbol s."},
       true,
       false},

  }));
  DYNAMIC_SECTION(name) { smoke(source, errors, useDriver, useOSFeats); }
}