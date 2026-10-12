/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/asmb_riscv/pseudo_instructions.hpp"
#include <memory>
#include <vector>
#include "core/compile/macro/macro_registry.hpp"

namespace {
const std::vector<pepp::tc::MacroDefinition> &definitions() {
  static const std::vector<pepp::tc::MacroDefinition> ret{};
  return ret;
}
} // namespace

void pepp::tc::riscv_insert_pseudo_instructions(MacroRegistry &registry) {
  // Fails, keeping the existing macro, if the name is already defined.
  for (const auto &definition : definitions()) (void)registry.insert(std::make_shared<MacroDefinition>(definition));
}
