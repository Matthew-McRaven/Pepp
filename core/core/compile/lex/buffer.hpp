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
#include <vector>
#include "core/math/bitmanip/span.hpp"
#include "core/compile/lex/tokens.hpp"

namespace pepp::tc::lex {

struct Token;
struct ALexer;
class Checkpoint;

class Buffer {
public:
  explicit Buffer(ALexer *lex);
  std::shared_ptr<Token> match(int mask);
  template <typename Type>
    requires(std::derived_from<Type, Token> && requires { std::integral_constant<int, Type::TYPE>{}; })
  std::shared_ptr<Type> match() {
    static constexpr auto mask = Type::TYPE;
    auto next = peek();
    if (next && next->mask(mask)) {
      _head++;
      return static_pointer_cast<Type>(next);
    }
    return nullptr;
  }
  template <typename... Types>
    requires((std::derived_from<Types, Token> && ...) &&
             (requires { std::integral_constant<int, Types::TYPE>{}; } && ...))
  std::shared_ptr<Token> match_not() {
    // Takes the bitwise NOT of the combined mask of all types.
    constexpr int combined = ~(Types::TYPE | ...);
    return match(combined);
  }
  std::shared_ptr<Token> match_not(int mask) { return match(~mask); }
  std::shared_ptr<Token> match_literal(const std::string &);
  // Returns the next token if it matches the mask, otherwise returns nullptr.
  std::shared_ptr<Token> peek(int mask = -1);
  template <typename Type>
    requires(std::derived_from<Type, Token> && requires { std::integral_constant<int, Type::TYPE>{}; })
  std::shared_ptr<Type> peek() {
    auto next = peek(Type::TYPE);
    if (next) return std::static_pointer_cast<Type>(next);
    else return nullptr;
  }
  std::shared_ptr<Token> peek_literal(const std::string &);
  bool input_remains() const;
  size_t count_buffered_tokens() const;
  size_t count_matched_tokens() const;
  // Return all tokens in the buffer between checkpoints head and our head.
  bits::span<std::shared_ptr<Token> const> matched_tokens_after(const Checkpoint &) const;
  bits::span<std::shared_ptr<Token> const> matched_tokens() const;
  support::LocationInterval matched_interval() const;
  // In some instances, the parser bypasses the token buffer to consume tokens directly from the lexer.
  // Sometimes we read one too many tokens in this mode and those tokens need to be re-buffered.
  void push_token(std::shared_ptr<Token> t);

private:
  ALexer *_lex;
  std::vector<std::shared_ptr<Token>> _tokens;
  size_t _head = 0, _checkpoints = 0;
  friend class Checkpoint;
  void clear_tokens();
};

// Effectively a semaphor for the buffer? As long as one checkpoint exists, Buffer's tokens will not be cleared.
// When the last checkpoint is destroyed, this CP will clear out old tokens and reset its head.
// This allows backtracking in the parser without having to store the entire program at once as tokens.
class Checkpoint {
public:
  explicit Checkpoint(Buffer &buf);
  ~Checkpoint();
  Checkpoint(const Checkpoint &) = delete;
  Checkpoint(Checkpoint &&) = default;
  Checkpoint &operator=(const Checkpoint &) = delete;
  Checkpoint &operator=(Checkpoint &&) = delete;

  // Reset the buffer to the state it was in when this checkpoint was created.
  void rollback();
  // Set this checkpoint's head to the buffer's current head.
  void commit();

private:
  friend class Buffer;
  Buffer &_buf;
  size_t _head = 0;
};

} // namespace pepp::tc::lex
