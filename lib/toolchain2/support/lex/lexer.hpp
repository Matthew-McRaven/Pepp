#pragma once
#include <QtCore/qobject.h>
#include <QtCore/qregularexpression.h>
#include <memory>
#include <vector>
#include "toolchain2/support/source/seekable.hpp"

namespace pepp::tc::support {
class StringPool;
}
namespace pepp::tc::lex {
struct Token;

struct ALexer {
  struct Listener {
    virtual ~Listener() = default;
    virtual void consumed(std::shared_ptr<Token> t) = 0;
  };
  ALexer(std::shared_ptr<support::StringPool> identifier_pool, support::SeekableData &&data);
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
  std::shared_ptr<support::StringPool> _pool;
};

} // namespace pepp::tc::lex
