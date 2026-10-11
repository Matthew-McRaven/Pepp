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
#pragma once
#include <cstddef>
#include <expected>
#include <functional>
#include <vector>

namespace pepp::tc::lex {
struct Token;
}

namespace pepp::tc::parser {

// Nesting of .IF / .ELSEIF / .ELSE / .ENDIF while parsing, and whether the current line is in a taken branch
class Conditionals {
public:
  enum class Error { UnmatchedElseif, UnmatchedElse, MultipleElse, UnmatchedEndif };
  void on_if(bool holds);
  // condition is only evaluated if no earlier branch was taken, so an untaken .ELSEIF need not be constant.
  std::expected<void, Error> on_elseif(const std::function<bool()> &condition);
  std::expected<void, Error> on_else();
  std::expected<void, Error> on_endif();

  // True while inside a branch which was not taken. Its lines must be skipped rather than parsed.
  bool skipping() const;
  std::size_t depth() const { return _levels.size(); }
  // Track nesting for a token read while skipping from depth. Returns true if the token may end the skip: an .ELSEIF or
  // .ELSE at depth when no branch has been taken yet, or the .ENDIF closing depth. That token must then be parsed.
  bool resumes_at(const lex::Token &token, std::size_t depth);

private:
  struct Level {
    bool matched_any = false;  // A branch at this level was taken, so no later .ELSEIF / .ELSE may be.
    bool matched_this = false; // The current branch is the one taken.
    bool matched_else = false; // An .ELSE was seen, so no .ELSEIF / .ELSE may follow.
  };
  std::vector<Level> _levels;
};

} // namespace pepp::tc::parser
