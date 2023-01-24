//
// Created by matthew on 8/2/22.
//
#include "ex_registry.hpp"

#include <filesystem>
#include <memory>
#include <boost/regex.hpp>

namespace fs = std::filesystem;

// Helper that find .db files in a given dir, and attempts to construct books from these DBs.
std::vector<std::shared_ptr<const Book>> explore_directory(const std::string &dir_name) {
  std::vector<std::shared_ptr<const Book>> ret;
  static const boost::regex reg(".*\\.db");
  for (fs::recursive_directory_iterator it(dir_name); it != fs::recursive_directory_iterator(); ++it) {
    if (boost::regex_search(it->path().generic_string(), reg)) {
      try {
        ret.emplace_back(std::make_shared<const Book>(it->path().generic_string()));
      }
      catch (std::exception &e) {
      }
    }
  }
  return ret;

}

registry::registry() {
  _books = explore_directory(fs::current_path());
}

registry::registry(std::vector<std::string> dirs) {
  for (auto &dir : dirs) {
    auto temp = explore_directory(dir);
    using std::begin, std::end;
    _books.insert(end(_books), std::begin(temp), std::end(temp));
  }
}

std::optional<std::shared_ptr<const Book>> registry::find_book(std::string name) const {
  auto find = std::find_if(_books.begin(), _books.end(), [&name](auto item) { return (item)->name() == name; });
  if (find == _books.cend())
    return std::nullopt;
  else
    return *find;
}

const std::vector<std::shared_ptr<const Book>> &registry::books() const {
  return _books;
}
