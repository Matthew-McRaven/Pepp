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
#include "core/langs/asmb/conditionals.hpp"
#include <algorithm>
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/math/bitmanip/strings.hpp"

void pepp::tc::parser::Conditionals::on_if(bool holds) {
  _levels.emplace_back(Level{.matched_any = holds, .matched_this = holds, .matched_else = false});
}

std::expected<void, pepp::tc::parser::Conditionals::Error>
pepp::tc::parser::Conditionals::on_elseif(const std::function<bool()> &condition) {
  if (_levels.empty() || _levels.back().matched_else) return std::unexpected(Error::UnmatchedElseif);
  auto &tos = _levels.back();
  if (tos.matched_any) tos.matched_this = false;
  else tos.matched_any = tos.matched_this = condition();
  return {};
}

std::expected<void, pepp::tc::parser::Conditionals::Error> pepp::tc::parser::Conditionals::on_else() {
  if (_levels.empty()) return std::unexpected(Error::UnmatchedElse);
  auto &tos = _levels.back();
  if (tos.matched_else) return std::unexpected(Error::MultipleElse);
  tos.matched_this = !tos.matched_any;
  tos.matched_any = tos.matched_else = true;
  return {};
}

std::expected<void, pepp::tc::parser::Conditionals::Error> pepp::tc::parser::Conditionals::on_endif() {
  if (_levels.empty()) return std::unexpected(Error::UnmatchedEndif);
  _levels.pop_back();
  return {};
}

bool pepp::tc::parser::Conditionals::skipping() const {
  return std::ranges::any_of(_levels, [](const Level &level) { return !level.matched_this; });
}

bool pepp::tc::parser::Conditionals::resumes_at(const lex::Token &token, std::size_t depth) {
  if (token.type() != lex::DotCommand::TYPE) return false;
  const auto dot = bits::to_upper(token.to_string());
  // A nested conditional inside a skipped branch is skipped entirely but still needs to be counted due to endif.
  if (dot == "IF") _levels.emplace_back(Level{.matched_any = false, .matched_this = false, .matched_else = false});
  else if (dot == "ELSEIF" || dot == "ELSE") return depth == _levels.size() && !_levels.back().matched_any;
  else if (dot == "ENDIF") {
    if (depth < _levels.size()) _levels.pop_back();
    else return true;
  }
  return false;
}
