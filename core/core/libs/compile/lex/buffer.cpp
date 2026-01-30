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

#include "core/libs/compile/lex/buffer.hpp"
#include "core/libs/compile/lex/lexer.hpp"

pepp::tc::lex::Buffer::Buffer(ALexer *lex) : _lex(lex), _tokens(), _head(0) {}

bool pepp::tc::lex::Buffer::input_remains() const {
  // If we have an unmatched token then input remains, otherwise delegate to lexer.
  // Failure to test head causes peek()'ing the last token to cause input_remains to be false.
  return _head < _tokens.size() || _lex->input_remains();
}

size_t pepp::tc::lex::Buffer::count_buffered_tokens() const { return _tokens.size(); }

size_t pepp::tc::lex::Buffer::count_matched_tokens() const { return _head; }

pepp::tc::support::LocationInterval pepp::tc::lex::Buffer::matched_interval() const {
  auto toks = matched_tokens();
  if (toks.empty()) return support::LocationInterval();
  auto lb = toks[0]->location().lower();
  auto ub = toks.last(1)[0]->location().upper();
  return support::LocationInterval(lb, ub);
}

bits::span<std::shared_ptr<pepp::tc::lex::Token> const> pepp::tc::lex::Buffer::matched_tokens() const {
  return bits::span<std::shared_ptr<pepp::tc::lex::Token> const>(_tokens.cbegin(), _tokens.cbegin() + _head);
}

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

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::Buffer::match_literal(const std::string &l) {
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

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::Buffer::peek_literal(const std::string &l) {
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
  // If there are no outstanding checkpoints, rollback is now impossible, so we can clear out old tokens.
  if (_buf._checkpoints == 0) _buf.clear_tokens();
}

void pepp::tc::lex::Checkpoint::rollback() { _buf._head = _head; }
