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
#include <stack>
#include "core/langs/asmb/asmb_lexer.hpp"
namespace pepp::tc::lex {

struct PepLexer : public AsmbLexer {
  PepLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, support::SeekableData &&data);
  ~PepLexer() override = default;
  PepLexer(const PepLexer &) = delete;
  PepLexer &operator=(const PepLexer &) = delete;
  PepLexer(PepLexer &&) noexcept = default;
  PepLexer &operator=(PepLexer &&) = default;
  static pepp::tc::lex::AsmbOptions options();
};

} // namespace pepp::tc::lex
