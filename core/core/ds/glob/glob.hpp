#pragma once
/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 * File extracted from the mold linker from v2.42.1 (a tag). Includes and namespaces were
 * adjusted to fit this project.
 *
 * MIT License
 * Copyright (c) 2023 Rui Ueyama
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <bitset>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "core/ds/glob/aho_corasick.hpp"
#include "core/integers.h"

// This file implements the glob matcher used for symbol name patterns.
// Exact, prefix and suffix patterns are matched directly, and simple
// substring patterns are combined into an Aho-Corasick matcher. The
// remaining patterns are matched with the non-recursive algorithm described
// at https://research.swtch.com/glob. If there are many such patterns, a
// bit-parallel NFA matches them together.
class Glob {
public:
  bool add(std::string_view pat, i64 val);
  bool empty() const { return is_empty; }
  i64 find(std::string_view str);

private:
  std::once_flag once;
  bool is_empty = true;
  bool is_compiled = false;
  i64 max_value = -1;

  // Patterns that need only a literal string comparison are kept out
  // of the automaton-based matchers below, which scan the entire input
  // string per query. Real version scripts consist almost entirely of
  // such patterns (e.g. `local: *;` or `v8dbg_*;`), and we match them
  // against every defined symbol name.
  struct LiteralPattern {
    std::string pat;
    i64 value;
  };

  struct Pattern {
    enum Kind { STRING, STAR, QUESTION, BRACKET };

    struct Token {
      Token(Kind kind) : kind(kind) {}

      Kind kind;
      std::string str;
      std::bitset<256> chars;
    };

    Pattern(std::vector<Token> &&tokens, i64 value) : tokens(std::move(tokens)), value(value) {}

    static std::optional<Pattern> compile(std::string_view pat, i64 value);
    bool match(std::string_view str) const;

    std::vector<Token> tokens;
    i64 value;
  };

  // Nfa matches many glob patterns in parallel by representing each state
  // with one bit. It is used only for large pattern sets; matching individual
  // patterns is faster when there are only a few of them.
  struct Nfa {
    void compile(std::span<const Pattern> patterns);
    i64 match(std::string_view str) const;
    bool empty() const { return initial_states.empty(); }

    std::vector<u64> initial_states;
    std::vector<u64> star_states;
    std::vector<u64> accept_states;
    std::vector<u64> char_masks;
    std::vector<i64> values;
  };

  i64 match_all = -1;                   // "*"
  std::vector<LiteralPattern> exacts;   // "foo"
  std::vector<LiteralPattern> prefixes; // "foo*"
  std::vector<LiteralPattern> suffixes; // "*foo"
  std::vector<Pattern> patterns;        // "foo*bar"

  Nfa nfa;
  AhoCorasick aho_corasick;
};
