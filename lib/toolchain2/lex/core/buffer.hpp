#pragma once
#include "./common_tokens.hpp"

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
  std::shared_ptr<Token> match_literal(QString);
  // Returns the next token if it matches the mask, otherwise returns nullptr.
  std::shared_ptr<Token> peek(int mask = -1);
  template <typename Type>
    requires(std::derived_from<Type, Token> && requires { std::integral_constant<int, Type::TYPE>{}; })
  std::shared_ptr<Type> peek() {
    auto next = peek(Type::TYPE);
    if (next) return std::static_pointer_cast<Type>(next);
    else return nullptr;
  }
  std::shared_ptr<Token> peek_literal(QString);
  bool input_remains() const;
  size_t buffered_tokens() const;
  size_t matched_tokens() const;

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
struct Checkpoint {
  explicit Checkpoint(Buffer &buf);
  ~Checkpoint();
  Checkpoint(const Checkpoint &) = delete;
  Checkpoint(Checkpoint &&) = default;
  Checkpoint &operator=(const Checkpoint &) = delete;
  Checkpoint &operator=(Checkpoint &&) = delete;

  // Reset the buffer to the state it was in when this checkpoint was created.
  void rollback();

private:
  Buffer &_buf;
  size_t _head = 0;
};

} // namespace pepp::tc::lex
