/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
#include <memory>
#include <optional>
#include <string_view>
#include "core/compile/ir_linear/attr_argument.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/langs/asmb_pep/ir_attributes.hpp"

namespace pepp::tc {
enum class PepIRType : int { Monadic = static_cast<int>(LinearIRType::FirstUser), Dyadic, DotAnnotate };

struct MonadicInstruction : public LinearIR {
  static constexpr int TYPE = static_cast<int>(PepIRType::Monadic);
  explicit MonadicInstruction(PepMnemonic m) : mnemonic(m) {}
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;
  int type() const override;
  PepMnemonic mnemonic;
};

struct DyadicInstruction : public LinearIR {
  static constexpr int TYPE = static_cast<int>(PepIRType::Dyadic);
  DyadicInstruction(PepMnemonic m, PepAddrMode am, Argument arg) : mnemonic(m), addr_mode(am), argument(arg) {}
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;
  int type() const override;
  PepMnemonic mnemonic;
  PepAddrMode addr_mode;
  Argument argument;
};

// Values of an instruction's operands as parsed.
struct PepParsedOperands {
  std::shared_ptr<pepp::ast::IRValue> value;
  pep::AddrMode mode = pep::AddrMode::INVALID;
};

// Select the subclass based on the descriptor.
std::shared_ptr<LinearIR> make_instruction(std::string_view name, const pep::MnemonicDescriptor &mn,
                                           const PepParsedOperands &operands);

struct DotAnnotate : public LinearIR {
  static constexpr int TYPE = static_cast<int>(PepIRType::DotAnnotate);
  enum class Which { END, EXPORT, INPUT, OUTPUT, SCALL } which;
  // Arg is an identifier, except for END, which has none
  DotAnnotate(Which dir, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  Argument argument;
};

bool defines_symbol(const LinearIR &line);

} // namespace pepp::tc
