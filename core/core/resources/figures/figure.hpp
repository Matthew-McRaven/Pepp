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

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "core/architectures.hpp"

namespace pepp {
class Fragment;
class Test;
class Figure {
public:
  Figure(pepp::Architecture arch, pepp::Abstraction level, pepp::Features features, std::string prefix,
         std::string chapter, std::string figure, bool is_problem = false);
  pepp::Architecture arch() const;
  pepp::Abstraction level() const;
  pepp::Features features() const;

  std::string name_prefix() const;
  std::string name_chapter() const;
  std::string name_figure() const;
  std::string name_full() const;

  std::string description() const;
  void set_description(std::string description);

  bool is_problem() const;

  bool is_os() const;
  void set_is_os(bool value);

  bool is_hidden() const;
  void set_is_hidden(bool value);

  const Figure *default_os() const;
  void set_default_os(const Figure *figure);

  const std::list<const Test *> &tests() const;
  void add_test(const Test *test);

  bool has_fragment(const std::string &name) const;
  const Fragment *find_fragment(const std::string &name) const;
  const std::vector<const Fragment *> &fragments() const;
  const std::map<std::string, const Fragment *> &named_fragments() const;
  // Transfer ownership to this. Must be deleted in this object's destructor
  bool add_fragment(const Fragment *fragment);
  std::string default_fragment_name() const;
  void set_default_fragment_name(std::string name);

  std::string default_fragment_text() const;
  std::string default_os_text() const;

private:
  const pepp::Architecture _arch;
  const pepp::Abstraction _level;
  const pepp::Features _features;
  const std::string _prefix, _name_chapter, _name_figure;
  const bool _is_problem;
  std::string _description;
  bool _is_os = false, _is_hidden = false;
  const Figure *_default_os = nullptr;
  std::list<const Test *> _tests = {};
  std::map<std::string, const Fragment *> _named_fragments = {};
  std::vector<const Fragment *> _all_fragments = {};
  std::string _default_fragment_name = {};
};

} // namespace pepp