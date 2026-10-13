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
#include "core/langs/asmb/codegen_error.hpp"
#include "core/macros.hpp"
#include "fmt/format.h"

const std::string pepp::tc::CodegenError::to_string(NullaryError err) {
  switch (err) {
  case NullaryError::Argument_OddOffset: return "Branch and jump offsets must be even";
  case NullaryError::Argument_NotRelocatable: return "This use of a symbol cannot be relocated";
  }
  PEPP_UNREACHABLE();
}

const std::string pepp::tc::CodegenError::to_string(UnaryError err, const std::string &arg) {
  switch (err) {
  case UnaryError::Argument_ValueDoesNotFit: return fmt::format("Argument does not fit in {}", arg);
  case UnaryError::Argument_AddressDoesNotFit: return fmt::format("A symbol's address does not fit in {}", arg);
  case UnaryError::Expression_Invalid: return fmt::format("Invalid expression: {}", arg);
  }
  PEPP_UNREACHABLE();
}

pepp::tc::CodegenError::CodegenError(NullaryError err, support::LocationInterval loc)
    : std::logic_error(to_string(err)), loc(loc) {}

pepp::tc::CodegenError::CodegenError(UnaryError err, std::string arg1, support::LocationInterval loc)
    : std::logic_error(to_string(err, arg1)), loc(loc) {}
