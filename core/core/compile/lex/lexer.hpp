/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
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

#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include "core/compile/source/location.hpp"
#include "core/compile/source/seekable.hpp"

namespace pepp::tc::lex {
struct Token;

struct ALexer {
  // references/pointers to *elements* are never invalidated unless removed from the container.
  // iterators may be invalidated in many cases. So, if derived lexer's return pointers to constant QStrings,
  // we get "cheap" (1 ptr wide) identifiers as well as reducing the average memory usage of the lexer.
  ALexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, support::SeekableData &&data);
  virtual ~ALexer() = default;
  virtual bool input_remains() const = 0;
  // Any Buffer should release the token as soon as it is done with it.
  virtual std::shared_ptr<Token> next_token() = 0;
  // If you received an invalid/error token, call this method to advance to the next point where we can resume
  // tokenization. This will return the interval which was skipped over.
  // The default behavior is to read until the next newline.
  virtual support::LocationInterval synchronize();

  support::Location current_location() const;
  const support::SeekableData &cursor() const { return _cursor; }
  // When using nested parsers (e.g., an expression parser in the assembler), this method allows the main lexer to
  // resume where the nested lexer stopped.
  void resume_at(support::SeekableData cursor) { _cursor = std::move(cursor); }
  // Indicate to lexer that it should print out each token as it is lexed.
  bool print_tokens = false;

protected:
  support::SeekableData _cursor;
  std::shared_ptr<std::unordered_set<std::string>> _pool;
};

} // namespace pepp::tc::lex
