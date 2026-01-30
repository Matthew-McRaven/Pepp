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
#include <vector>
#include "core/libs/compile/source/location.hpp"
#include "core/libs/compile/source/seekable.hpp"

namespace pepp::tc::lex {
struct Token;

struct ALexer {
  struct Listener {
    virtual ~Listener() = default;
    virtual void consumed(std::shared_ptr<Token> t) = 0;
  };
  // references/pointers to *elements* are never invalidated unless removed from the container.
  // iterators may be invalidated in many cases. So, if derived lexer's return pointers to constant QStrings,
  // we get "cheap" (1 ptr wide) identifiers as well as reducing the average memory usage of the lexer.
  ALexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, support::SeekableData &&data);
  virtual ~ALexer() = default;
  virtual bool input_remains() const = 0;
  // Whenever next_token is called, must notify all listeners via Listener::consumed.
  // I originally wanted to return a unique_ptr here, but I don't know how long listeners will keep tokens alive.
  // In the case of a source formatter, it may want to keep many lines worth of tokens alive.
  // That being said, any Buffer/Listener should release the token as soon as they are done with it.
  virtual std::shared_ptr<Token> next_token() = 0;
  // If you received an invalid/error token, call this method to advance to the next point where we can resume
  // tokenization. This will return the interval which was skipped over.
  // The default behavior is to read until the next newline.
  virtual support::LocationInterval synchronize();

  void register_listener(Listener *listener);
  support::Location current_location() const;
  // Indicate to lexer that it should print out each token as it is lexed.
  // TODO: replace with a listener that does the same thing.
  bool print_tokens = false;

protected:
  std::vector<Listener *> _listeners;
  void notify_listeners(std::shared_ptr<Token> t);
  support::SeekableData _cursor;
  std::shared_ptr<std::unordered_set<std::string>> _pool;
};

} // namespace pepp::tc::lex
