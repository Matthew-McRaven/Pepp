//
// Created by matthew on 8/2/22.
//
#pragma once

#include <memory>

#include "book.hpp"

class registry {
public:
  // Explore CWD for any database files
  registry();
  // Explore only the provided directories. Does not include CWD unless explicitly passed.
  explicit registry(std::vector<std::string> dirs);
  std::optional<std::shared_ptr<const Book>> find_book(std::string name) const;
  const std::vector<std::shared_ptr<const Book>> &books() const;
private:
  std::vector<std::shared_ptr<const Book>> _books;
};
