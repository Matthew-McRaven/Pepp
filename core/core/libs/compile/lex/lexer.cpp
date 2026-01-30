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

#include "./lexer.hpp"

pepp::tc::lex::ALexer::ALexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                              support::SeekableData &&data)
    : _cursor(std::move(data)), _pool(identifier_pool) {}

pepp::tc::support::LocationInterval pepp::tc::lex::ALexer::synchronize() {
  auto start = _cursor.location();
  while (input_remains()) {
    if (auto next = _cursor.peek(); next == '\n') {
      _cursor.advance(1);
      _cursor.newline();
      break;
    } else _cursor.advance(1);
  }
  return {start, _cursor.location()};
}

void pepp::tc::lex::ALexer::register_listener(Listener *listener) {
  // Do not allow registering null or duplicate listeners.
  if (listener == nullptr || std::find(_listeners.cbegin(), _listeners.cend(), listener) != _listeners.cend()) return;
  _listeners.push_back(listener);
}

pepp::tc::support::Location pepp::tc::lex::ALexer::current_location() const { return _cursor.location(); }

void pepp::tc::lex::ALexer::notify_listeners(std::shared_ptr<Token> t) {
  for (const auto &l : _listeners) l->consumed(t);
}
