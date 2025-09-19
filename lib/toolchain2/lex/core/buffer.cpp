#include "./buffer.hpp"
#include "./lexer.hpp"
pepp::tc::lex::Buffer::Buffer(ALexer *lex) : _lex(lex), _tokens(), _head(0) {}

bool pepp::tc::lex::Buffer::input_remains() const {
  // If we have an unmatched token then input remains, otherwise delegate to lexer.
  // Failure to test head causes peek()'ing the last token to cause input_remains to be false.
  return _head < _tokens.size() || _lex->input_remains();
}

size_t pepp::tc::lex::Buffer::buffered_tokens() const { return _tokens.size(); }
size_t pepp::tc::lex::Buffer::matched_tokens() const { return _head; }

void pepp::tc::lex::Buffer::clear_tokens() {
  _tokens.clear();
  _head = 0;
  // TODO: notify listeners than tokens have been destroyed and they should clear them too.
  // This would also be a good place to notify the tokenizer to read in the next chunk of data if not already in main
  // memory.
}

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::Buffer::match(int mask) {
  if (auto next = peek(); next && next->mask(mask)) return _head++, next;
  else return nullptr;
}

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::Buffer::match_literal(QString l) {
  using T = pepp::tc::lex::CommonTokenType;
  if (auto next = peek(); next && next->mask((int)T::Literal) && std::static_pointer_cast<Literal>(next)->literal == l)
    return _head++, next;
  else return nullptr;
}

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::Buffer::peek(int mask) {
  if (_head == _tokens.size()) {
    // Do not append a nullptr, otherwise head will be < size even though we reached EoF;
    if (auto next = _lex->next_token(); !next) return nullptr;
    else _tokens.emplace_back(next);
  }

  if (auto l = _tokens[_head]; l && l->mask(mask)) return l;
  return nullptr;
}

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::Buffer::peek_literal(QString l) {
  using T = pepp::tc::lex::CommonTokenType;

  if (auto next = peek(); next && next->mask((int)T::Literal) && std::static_pointer_cast<Literal>(next)->literal == l)
    return next;
  else return nullptr;
}

pepp::tc::lex::Checkpoint::Checkpoint(Buffer &buf) : _buf(buf) {
  _head = _buf._head;
  _buf._checkpoints++;
}

pepp::tc::lex::Checkpoint::~Checkpoint() {
  _buf._checkpoints--;
  // If there are no outstanding checkpoints, rollback is now imossible, so we can clear out old tokens.
  if (_buf._checkpoints == 0) _buf.clear_tokens();
}

void pepp::tc::lex::Checkpoint::rollback() { _buf._head = _head; }
