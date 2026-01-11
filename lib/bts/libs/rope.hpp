//
// Copyright Will Roever 2016 - 2017.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace pepp::bts {
// A rope represents a string as a binary tree wherein the leaves contain fragments of the
//   string. More accurately, a rope consists of a pointer to a root rope_node, which
//   describes a binary tree of string fragments.

// Examples:
//
//        X        |  null root pointer, represents empty string
//
//      "txt"      |
//     /     \     |  root is a leaf node containing a string fragment
//    X       X    |
//
//        O        |
//       / \       |
//  "some" "text"  |  root is an internal node formed by the concatenation of two distinct
//    /\     /\    |  ropes containing the strings "some" and "text"
//   X  X   X  X   |

class rope {

public:
  // A rope_node represents a string as a binary tree of string fragments
  //
  // A rope_node consists of:
  //   - a non-negative integer weight
  //   - a pointer to a left child rope_node
  //   - a pointer to a right child rope_node
  //   - a string fragment
  //
  // INVARIANTS:
  //   - a leaf is represented as a rope_node with null child pointers
  //   - a leaf node's weight is equal to the length of the string fragment it contains
  //   - an internal node is represented as a rope_node with non-null children and
  //     an empty string fragment
  //   - an internal node's weight is equal to the length of the string fragment
  //     contained in (the leaf nodes of) its left subtree

  class node {
  public:
    // CONSTRUCTORS
    // Construct internal node by concatenating the given nodes
    node(std::unique_ptr<node> l, std::unique_ptr<node> r);
    // Construct leaf node from the given string
    node(const std::string &str);
    // Copy constructor
    node(const node &);

    // ACCESSORS
    size_t getLength(void) const;
    char getCharByIndex(size_t) const;
    // Get the substring of (len) chars beginning at index (start)
    std::string getSubstring(size_t start, size_t len) const;
    // Get string contained in current node and its children
    std::string treeToString(void) const;

    // MUTATORS
    // Split the represented string at the specified index
    friend std::pair<std::unique_ptr<node>, std::unique_ptr<node>> splitAt(std::unique_ptr<node>, size_t);

    // HELPERS
    // Functions used in balancing
    size_t getDepth(void) const;
    void getLeaves(std::vector<node *> &v);

  private:
    // Determine whether a node is a leaf
    bool isLeaf(void) const;

    size_t weight_;
    std::unique_ptr<node> left_;
    std::unique_ptr<node> right_;
    std::string fragment_;

  }; // class rope_node

  // CONSTRUCTORS
  // Default constructor - produces a rope representing the empty string
  rope(void);
  // Construct a rope from the given string
  rope(const std::string &);
  // Copy constructor
  rope(const rope &);

  // Get the string stored in the rope
  std::string toString() const;
  // Get the length of the stored string
  size_t length() const;
  // Get the character at the given position in the represented string
  char at(size_t index) const;
  // Return the substring of length (len) beginning at the specified index
  std::string substring(size_t start, size_t len) const;
  // Determine if rope is balanced
  bool isBalanced(void) const;
  // Balance the rope
  void balance(void);

  // MUTATORS
  // Insert the given string/rope into the rope, beginning at the specified index (i)
  void insert(size_t i, const std::string &str);
  void insert(size_t i, const rope &r);
  // Concatenate the existing string/rope with the argument
  void append(const std::string &);
  void append(const rope &);
  // Delete the substring of (len) characters beginning at index (start)
  void rdelete(size_t start, size_t len);

  // OPERATORS
  rope &operator=(const rope &rhs);
  bool operator==(const rope &rhs) const;
  bool operator!=(const rope &rhs) const;
  friend std::ostream &operator<<(std::ostream &out, const rope &r);

private:
  // Pointer to the root of the rope tree
  std::unique_ptr<node> root_;

}; // class rope

size_t fib(size_t n);
std::vector<size_t> buildFibList(size_t len);

} // namespace pepp::bts
