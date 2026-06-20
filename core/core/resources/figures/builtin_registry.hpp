/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
 *
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
 */

#pragma once

#include <any>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "core/architectures.hpp"

namespace pepp {
static const char *default_builtins_path = "/";
class Book;
class Figure;
class Fragment;
class Test;

class BuiltinRegistry {
public:
  struct Assembler {
    virtual ~Assembler() = default;
    virtual std::any operator()(const std::string &os, const std::string &user) = 0;
  };
  struct Formatter {
    virtual ~Formatter() = default;
    virtual std::string operator()(std::any assembled) = 0;
  };
  struct FilesystemProvider {
    virtual ~FilesystemProvider() = default;
    virtual std::string dir_of(const std::string &path, const std::string &parent = "") const = 0;
    virtual std::string read_file(const std::string &path, const std::string &parent = "") const = 0;
    // Return the relative paths of all files and subdirectories within the given directory. Does not recurse.
    virtual std::vector<std::string> enumerate_files(const std::string &directory) const = 0;
    // Return true if these are not the "compiled in" figures.
    virtual bool using_external_figures() const = 0;
  };
  // Crawling the filesystem create books is handled inside CTOR.
  explicit BuiltinRegistry(std::unique_ptr<FilesystemProvider> fs);
  std::list<std::shared_ptr<const pepp::Book>> books() const;
  std::shared_ptr<const pepp::Book> find_book(std::string name) const;
  bool using_external_figures() const { return _fs->using_external_figures(); }
  void add_dependency(const Fragment *dependent, const Fragment *dependee);
  std::string content_for(Fragment &fragment);
  void add_assembler(pepp::Architecture arch, std::unique_ptr<Assembler> &&assembler);
  void add_formatter(pepp::Architecture arch, std::string format, std::unique_ptr<Formatter> &&formatter);
private:
  std::shared_ptr<Book> load_book(const std::string &toc_path);
  void link_figure_to_OS(const std::string &manifest_path, std::shared_ptr<Figure> figure,
                         std::shared_ptr<const Book> book);

  std::unique_ptr<FilesystemProvider> _fs;
  std::list<std::shared_ptr<const pepp::Book>> _books;
  // Given an element, determine which element it depends on.
  std::map<const Fragment * /*dependent*/, const Fragment * /*dependee*/> _dependencies;
  // Given an element, determine which elements depend on it.
  std::map<const Fragment * /*dependee*/, std::vector<const Fragment *> /*dependents*/> _dependees;
  void compute_dependencies(const Fragment *dependee);
  std::map<const Fragment *, std::string> _contents;
  std::map<pepp::Architecture, std::unique_ptr<Assembler>> _assemblers;
  std::map<std::pair<pepp::Architecture, std::string>, std::unique_ptr<Formatter>> _formatters;
};

} // namespace pepp