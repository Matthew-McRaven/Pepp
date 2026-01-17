/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright Will Roever 2016 - 2017.
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */
#include "rope.hpp"
#include <iostream>

// out-of-bounds error constant
std::invalid_argument ERROR_OOB_ROPE = std::invalid_argument("Error: string index out of bounds");

// Default constructor - produces a rope representing the empty string
pepp::bts::rope::rope(void) : rope("") {}

// Construct a rope from the given string
pepp::bts::rope::rope(const std::string &str) { this->root_ = make_unique<node>(str); }

// Copy constructor
pepp::bts::rope::rope(const rope &r) { this->root_ = std::make_unique<node>(*r.root_); }

// Get the string stored in the rope
std::string pepp::bts::rope::toString() const {
  if (this->root_ == nullptr) return "";
  // TODO: flatten the nodes while we are at it
  return this->root_->treeToString();
}

// Get the length of the stored string
size_t pepp::bts::rope::length() const {
  if (this->root_ == nullptr) return 0;
  return this->root_->getLength();
}

// Get the character at the given position in the represented string
char pepp::bts::rope::at(size_t index) const {
  if (this->root_ == nullptr) throw ERROR_OOB_ROPE;
  return this->root_->getCharByIndex(index);
}

// Return the substring of length (len) beginning at the specified index
std::string pepp::bts::rope::substring(size_t start, size_t len) const {
  size_t actualLength = this->length();
  if (start > actualLength || (start + len) > actualLength) throw ERROR_OOB_ROPE;
  return this->root_->getSubstring(start, len);
}

// Insert the given string into the rope, beginning at the specified index (i)
void pepp::bts::rope::insert(size_t i, const std::string &str) { this->insert(i, rope(str)); }

// Insert the given rope into the rope, beginning at the specified index (i)
void pepp::bts::rope::insert(size_t i, const rope &r) {
  if (this->length() < i) {
    throw ERROR_OOB_ROPE;
  } else {
    rope tmp = rope(r);
    auto origRopeSplit = splitAt(std::move(this->root_), i);
    auto tmpConcat = make_unique<node>(std::move(origRopeSplit.first), std::move(tmp.root_));
    this->root_ = make_unique<node>(std::move(tmpConcat), std::move(origRopeSplit.second));
  }
}

// Append the argument to the existing rope
void pepp::bts::rope::append(const std::string &str) {
  rope tmp = rope(str);
  this->root_ = make_unique<node>(std::move(this->root_), std::move(tmp.root_));
}

// Append the argument to the existing rope
void pepp::bts::rope::append(const rope &r) {
  rope tmp = rope(r);
  this->root_ = make_unique<node>(std::move(this->root_), std::move(tmp.root_));
}

// Delete the substring of (len) characters beginning at index (start)
void pepp::bts::rope::rdelete(size_t start, size_t len) {
  size_t actualLength = this->length();
  if (start > actualLength || start + len > actualLength) {
    throw ERROR_OOB_ROPE;
  } else {
    auto firstSplit = splitAt(std::move(this->root_), start);
    auto secondSplit = splitAt(std::move(firstSplit.second), len);
    secondSplit.first.reset();
    this->root_ = make_unique<node>(std::move(firstSplit.first), std::move(secondSplit.second));
  }
}

// Determine if rope is balanced
//
// A rope is balanced if and only if its length is greater than or equal to
//   fib(d+2) where d refers to the depth of the rope and fib(n) represents
//   the nth fibonacci number i.e. in the set {1,1,2,3,5,8,etc...}
bool pepp::bts::rope::isBalanced(void) const {
  if (this->root_ == nullptr) return true;
  size_t d = this->root_->getDepth();
  return this->length() >= fib(d + 2);
}

// Balance a rope
void pepp::bts::rope::balance(void) {
  // initiate rebalancing only if rope is unbalanced
  if (!this->isBalanced()) {
    // build vector representation of Fibonacci intervals
    std::vector<size_t> intervals = buildFibList(this->length());
    std::vector<std::unique_ptr<node>> nodes(intervals.size());

    // get leaf nodes
    std::vector<node *> leaves;
    this->root_->getLeaves(leaves);

    size_t i;
    size_t max_i = intervals.size() - 1;
    size_t currMaxInterval = 0;
    std::unique_ptr<node> acc = nullptr, tmp = nullptr;

    // attempt to insert each leaf into nodes vector based on length
    for (auto &leaf : leaves) {
      size_t len = leaf->getLength();
      bool inserted = false;

      // ignore empty leaf nodes
      if (len > 0) {
        acc = std::make_unique<node>(*leaf);
        i = 0;

        while (!inserted) {
          // find appropriate slot for the acc node to be inserted,
          // concatenating with nodes encountered along the way
          while (i < max_i && len >= intervals[i + 1]) {
            if (nodes[i].get() != nullptr) {
              // concatenate encountered entries with node to be inserted
              tmp = std::make_unique<node>(*nodes[i].get());
              acc = std::make_unique<node>(*acc.get());
              acc = std::make_unique<node>(std::move(tmp), std::move(acc));

              // update len
              len = acc->getLength();

              // if new length is sufficiently great that the node must be
              //   moved to a new slot, we clear out the existing entry
              if (len >= intervals[i + 1]) nodes[i] = nullptr;
            }
            i++;
          }

          // target slot found -- check if occupied
          if (nodes[i].get() == nullptr) {
            nodes[i].swap(acc);
            inserted = true;
            // update currMaxInterval if necessary
            if (i > currMaxInterval) currMaxInterval = i;
          } else {
            // concatenate encountered entries with node to be inserted
            tmp = std::make_unique<node>(*nodes[i].get());
            acc = std::make_unique<node>(*acc.get());
            acc = std::make_unique<node>(std::move(tmp), std::move(acc));

            // update len
            len = acc->getLength();

            // if new length is sufficiently great that the node must be
            //   moved to a new slot, we clear out the existing entry
            if (len >= intervals[i + 1]) nodes[i] = nullptr;
          }
        }
      }
    }

    // concatenate remaining entries to produce balanced rope
    acc = std::move(nodes[currMaxInterval]);
    for (int idx = currMaxInterval; idx >= 0; idx--) {
      if (nodes[idx] != nullptr) {
        tmp = std::make_unique<node>(*nodes[idx].get());
        acc = std::make_unique<node>(std::move(acc), std::move(tmp));
      }
    }
    this->root_ = std::move(acc); // reset root
  }
}

// Assignment operator
pepp::bts::rope &pepp::bts::rope::operator=(const rope &rhs) {
  // check for self-assignment
  if (this == &rhs) return *this;
  // delete existing rope to recover memory
  this->root_.reset();
  // invoke copy constructor
  this->root_ = std::make_unique<node>(*(rhs.root_.get()));
  return *this;
}

// Determine if two ropes contain identical strings
bool pepp::bts::rope::operator==(const rope &rhs) const { return this->toString() == rhs.toString(); }

// Determine if two ropes contain identical strings
bool pepp::bts::rope::operator!=(const rope &rhs) const { return !(*this == rhs); }

// Print the rope
std::ostream &pepp::bts::operator<<(std::ostream &out, const pepp::bts::rope &r) {
  // auto s = r.toString();
  // out.write(s.data(), static_cast<std::streamsize>(s.size()));
  // return out;
  return out << r.toString();
}

// Compute the nth Fibonacci number, in O(n) time
size_t pepp::bts::fib(size_t n) {
  // initialize first two numbers in sequence
  size_t a = 0, b = 1, next = 0;
  if (n == 0) return a;
  for (size_t i = 2; i <= n; i++) {
    next = a + b;
    a = b;
    b = next;
  }
  return b;
};

// Construct a vector where the nth entry represents the interval between the
//   Fibonacci numbers F(n+2) and F(n+3), and the final entry includes the number
//   specified in the length parameter
// e.g. buildFibList(0) -> {}
//      buildFibList(1) -> {[1,2)}
//      buildFibList(8) -> {[1,2),[2,3),[3,5),[5,8),[8,13)}
std::vector<size_t> pepp::bts::buildFibList(size_t len) {
  // initialize a and b to the first and second fib numbers respectively
  int a = 0, b = 1, next;
  std::vector<size_t> intervals = std::vector<size_t>();
  while (a <= len) {
    if (a > 0) intervals.push_back(b);

    next = a + b;
    a = b;
    b = next;
  }
  return intervals;
}

// Define out-of-bounds error constant
std::invalid_argument ERROR_OOB_NODE = std::invalid_argument("Error: string index out of bounds");

// Construct internal node by concatenating the given nodes
pepp::bts::rope::node::node(std::unique_ptr<node> l, std::unique_ptr<node> r) : fragment_("") {
  this->left_ = std::move(l);
  this->right_ = std::move(r);
  this->weight_ = this->left_->getLength();
}

// Construct leaf node from the given string
pepp::bts::rope::node::node(const std::string &str)
    : weight_(str.length()), left_(nullptr), right_(nullptr), fragment_(str) {}

// Copy constructor
pepp::bts::rope::node::node(const node &aNode) : weight_(aNode.weight_), fragment_(aNode.fragment_) {
  node *tmpLeft = aNode.left_.get(), *tmpRight = aNode.right_.get();
  if (tmpLeft == nullptr) this->left_ = nullptr;
  else this->left_ = std::make_unique<node>(*tmpLeft);
  if (tmpRight == nullptr) this->right_ = nullptr;
  else this->right_ = std::make_unique<node>(*tmpRight);
}

// Determine whether a node is a leaf
bool pepp::bts::rope::node::isLeaf(void) const { return this->left_ == nullptr && this->right_ == nullptr; }

// Get string length by adding the weight of the root and all nodes in
//   path to rightmost child
size_t pepp::bts::rope::node::getLength() const {
  if (this->isLeaf()) return this->weight_;
  size_t tmp = (this->right_ == nullptr) ? 0 : this->right_->getLength();
  return this->weight_ + tmp;
}

// Get the character at the given index
char pepp::bts::rope::node::getCharByIndex(size_t index) const {
  size_t w = this->weight_;
  // if node is a leaf, return the character at the specified index
  if (this->isLeaf()) {
    if (index >= this->weight_) throw ERROR_OOB_NODE;
    else return this->fragment_[index];
  } else { // else search the appropriate child node
    if (index < w) return this->left_->getCharByIndex(index);
    else return this->right_->getCharByIndex(index - w);
  }
}

// Get the substring of (len) chars beginning at index (start)
std::string pepp::bts::rope::node::getSubstring(size_t start, size_t len) const {
  size_t w = this->weight_;
  if (this->isLeaf()) {
    if (len < w) {
      return this->fragment_.substr(start, len);
    } else {
      return this->fragment_;
    }
  } else {
    // check if start index in left subtree
    if (start < w) {
      std::string lResult = (this->left_ == nullptr) ? "" : this->left_->getSubstring(start, len);
      if ((start + len) > w) {
        // get number of characters in left subtree
        size_t tmp = w - start;
        std::string rResult = (this->right_ == nullptr) ? "" : this->right_->getSubstring(w, len - tmp);
        return lResult.append(rResult);
      } else {
        return lResult;
      }
      // if start index is in the right subtree...
    } else {
      return (this->right_ == nullptr) ? "" : this->right_->getSubstring(start - w, len);
    }
  }
}

// Get string contained in current node and its children
std::string pepp::bts::rope::node::treeToString(void) const {
  if (this->isLeaf()) {
    return this->fragment_;
  }
  std::string lResult = (this->left_ == nullptr) ? "" : this->left_->treeToString();
  std::string rResult = (this->right_ == nullptr) ? "" : this->right_->treeToString();
  return lResult.append(rResult);
}

// Split the represented string at the specified index
std::pair<std::unique_ptr<pepp::bts::rope::node>, std::unique_ptr<pepp::bts::rope::node>>
pepp::bts::splitAt(std::unique_ptr<pepp::bts::rope::node> node, size_t index) {
  size_t w = node->weight_;
  // if the given node is a leaf, split the leaf
  if (node->isLeaf()) {
    return std::pair{std::make_unique<rope::node>(node->fragment_.substr(0, index)),
                     std::make_unique<rope::node>(node->fragment_.substr(index, w - index))};
  }

  // if the given node is a concat (internal) node, compare index to weight and handle accordingly
  auto oldRight = std::move(node->right_);
  if (index < w) {
    node->right_ = nullptr;
    node->weight_ = index;
    auto splitLeftResult = splitAt(std::move(node->left_), index);
    node->left_ = std::move(splitLeftResult.first);
    return std::pair{std::move(node), make_unique<rope::node>(std::move(splitLeftResult.second), std::move(oldRight))};
  } else if (w < index) {
    auto splitRightResult = splitAt(std::move(oldRight), index - w);
    node->right_ = std::move(splitRightResult.first);
    return std::pair{std::move(node), std::move(splitRightResult.second)};
  } else {
    return std::pair{std::move(node->left_), std::move(oldRight)};
  }
}

// Get the maximum depth of the rope, where the depth of a leaf is 0 and the
//   depth of an internal node is 1 plus the max depth of its children
size_t pepp::bts::rope::node::getDepth(void) const {
  if (this->isLeaf()) return 0;
  size_t lResult = (this->left_ == nullptr) ? 0 : this->left_->getDepth();
  size_t rResult = (this->right_ == nullptr) ? 0 : this->right_->getDepth();
  return std::max(++lResult, ++rResult);
}

// Store all leaves in the given vector
void pepp::bts::rope::node::getLeaves(std::vector<node *> &v) {
  if (this->isLeaf()) {
    v.push_back(this);
  } else {
    node *tmpLeft = this->left_.get();
    if (tmpLeft != nullptr) tmpLeft->getLeaves(v);
    node *tmpRight = this->right_.get();
    if (tmpRight != nullptr) tmpRight->getLeaves(v);
  }
}
