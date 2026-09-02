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

#include "core/arch/pep/isa/pep10.hpp"
#include "core/resources/figures/book.hpp"
#include "core/resources/figures/builtin_registry.hpp"
#include "help/builtins/figure_wrappers.hpp"
#include "sim3/subsystems/bus/simple.hpp"
#include "sim3/systems/traced_pep_isa3_system.hpp"
#include "toolchain/link/memmap.hpp"
#include "toolchain/link/mmio.hpp"
#include "toolchain/macro/declaration.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/obj/pep10.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"

static const auto is_charIn = [](const auto &x) {
  return x.name == "charIn" && x.type == obj::IO::Type::kInput && x.minOffset == 0xFFFD && x.maxOffset == 0xFFFD;
};
static const auto is_charOut = [](const auto &x) {
  return x.name == "charOut" && x.type == obj::IO::Type::kOutput && x.minOffset == 0xFFFE && x.maxOffset == 0xFFFE;
};
static const auto is_pwrOff = [](const auto &x) {
  return x.name == "pwrOff" && x.type == obj::IO::Type::kOutput && x.minOffset == 0xFFFF && x.maxOffset == 0xFFFF;
};

inline void loadBookMacros(std::shared_ptr<const pepp::Book> book, QSharedPointer<macro::Registry> registry) {
  for (auto &macro : book->macros()) {
    // TODO: hideous conversion from current book type to the old macro type. Refactor to remove this copy.
    const auto arch = pepp::arch_as_string(macro->arch);
    auto macroDecl = QSharedPointer<::macro::Declaration>::create(
        QString::fromStdString(macro->name), macro->argcount, QString::fromStdString(macro->body),
        QString::fromStdString(arch), QString::fromStdString(macro->family), macro->hidden);
    registry->registerMacro(::macro::types::Core, macroDecl);
  }
}

inline void injectFakeSCallMacros(QSharedPointer<macro::Registry> registry) {
  static const QStringList nonunary = {"DECI", "CHARI", "CHARO", "STRO", "DECO", "PRINTF", "HEXO"};
  for (auto &macro : nonunary)
    registry->registerMacro(macro::types::Core,
                            QSharedPointer<macro::Declaration>::create(macro, 2, "LDWA 0,i\nSCALL $1, $2", "pep/10"));
}
