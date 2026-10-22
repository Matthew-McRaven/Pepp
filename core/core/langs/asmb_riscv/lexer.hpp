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

#pragma once
#include "core/langs/asmb/asmb_lexer.hpp"

namespace pepp::langs {

struct RISCVLexer : public pepp::tc::lex::AsmbLexer {
  RISCVLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, tc::support::SeekableData &&data);
  ~RISCVLexer() override = default;
  RISCVLexer(const RISCVLexer &) = delete;
  RISCVLexer &operator=(const RISCVLexer &) = delete;
  RISCVLexer(RISCVLexer &&) noexcept = default;
  RISCVLexer &operator=(RISCVLexer &&) = default;

  static pepp::tc::lex::AsmbOptions options();
};
} // namespace pepp::langs
