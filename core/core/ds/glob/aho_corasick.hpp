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
#include <array>
#include <string_view>
#include <vector>
#include "core/integers.h"
// This file implements the Aho-Corasick algorithm to search multiple
// strings within an input string simultaneously. It is essentially a
// trie with additional links. For details, see
// https://en.wikipedia.org/wiki/Aho-Corasick_algorithm.
//
// We use it for simple glob patterns in version scripts or dynamic
// list files. Here are some examples of glob patterns:
//
//    qt_private_api_tag*
//    *16QAccessibleCache*
//    *32QAbstractFileIconProviderPrivate*
//    *17QPixmapIconEngine*
//
// Aho-Corasick can do only substring search, so it cannot handle
// complex glob patterns such as `*foo*bar*`. We handle such patterns
// with the Glob class.

class AhoCorasick {
public:
  bool add(std::string_view pat, i64 val);
  bool empty() const { return nodes.empty(); }
  void compile();
  i64 find(std::string_view str);

  static bool can_handle(std::string_view str);

private:
  struct TrieNode {
    i64 value = -1;
    i32 suffix_link = -1;
    i32 first_child = -1;
    i32 next_sibling = -1;
    u8 ch = 0;
  };

  i32 find_child(i32 node, u8 ch) const;
  i32 add_child(i32 node, u8 ch);

  // Most trie nodes have only one child. The root uses a dense table because
  // it is visited for almost every input byte; other edges are stored sparsely.
  std::array<i32, 256> root_children;
  std::vector<TrieNode> nodes;
  i64 max_value = -1;
};
