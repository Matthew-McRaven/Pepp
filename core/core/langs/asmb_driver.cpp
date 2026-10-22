/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include "asmb_driver.hpp"
#include <type_traits>
#include <utility>
#include "core/compile/symbol/value.hpp"

namespace pepp::tc {

DriverResult assemble(const DriverConfig &config, const FormattingConfig &fmt, std::string source) {
  const auto disp = [&](auto &&arch_config) -> DriverResult {
    using Config = std::decay_t<decltype(arch_config)>;
    if constexpr (std::is_same_v<Config, RISCVDriverConfig>) return assemble_riscv(arch_config, fmt, std::move(source));
    else if constexpr (std::is_same_v<Config, Pep10DriverConfig>)
      return assemble_pep10(arch_config, fmt, std::move(source));
    else if constexpr (std::is_same_v<Config, Pep9DriverConfig>)
      return assemble_pep9(arch_config, fmt, std::move(source));
  };
  return std::visit(disp, config);
}

std::expected<std::vector<std::string>, std::string> DriverResult::listing() {
  if (!unformatted_listing) return std::unexpected("no listing was requested, or assembly failed before codegen");
  const auto &[text, style] = *unformatted_listing;
  return format_listing(to_input_elf(*_elf), text, style);
}

void inject_symdefs(std::vector<std::pair<std::string, u32>> symdefs, core::symbol::LeafTable &symtab, u8 byte_width) {
  for (const auto &[name, value] : symdefs) {
    auto s = symtab.define(name);
    auto _bits = bits::MaskedBits{.byteCount = byte_width, .bitPattern = value, .mask = bits::mask(byte_width)};
    s->value = std::make_shared<core::symbol::ConstantValue>(_bits);
    s->binding = core::symbol::Binding::Weak;
  }
}

} // namespace pepp::tc
