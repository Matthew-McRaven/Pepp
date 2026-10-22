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

#include "core/langs/asmb_pep/lexer.hpp"

pepp::tc::lex::PepLexer::PepLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                  support::SeekableData &&data)
    : AsmbLexer(identifier_pool, std::move(data), options()) {}

pepp::tc::lex::AsmbOptions pepp::tc::lex::PepLexer::options() {
  return pepp::tc::lex::AsmbOptions{.allow_dot_in_ident = false,
                                    .allow_parens = false,
                                    .allow_macro_arguments = true,
                                    .allow_at_in_ident = true,
                                    .line_comment_leader = ";"};
}
