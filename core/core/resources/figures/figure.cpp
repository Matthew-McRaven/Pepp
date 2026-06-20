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
#include "figure.hpp"
#include <spdlog/spdlog.h>
#include "fragment.hpp"

pepp::Figure::Figure(Architecture arch, Abstraction level, Features feats, std::string prefix, std::string chapter,
                     std::string figure, bool is_problem)
    : _arch(arch), _level(level), _features(feats), _prefix(prefix), _name_chapter(chapter), _name_figure(figure),
      _is_problem(is_problem) {}

pepp::Architecture pepp::Figure::arch() const { return _arch; }

pepp::Abstraction pepp::Figure::level() const { return _level; }

pepp::Features pepp::Figure::features() const { return _features; }

std::string pepp::Figure::name_prefix() const { return _prefix; }

std::string pepp::Figure::name_chapter() const { return _name_chapter; }

std::string pepp::Figure::name_figure() const { return _name_figure; }

std::string pepp::Figure::name_full() const {
  if (_prefix.empty()) return fmt::format("{}.{}", _name_chapter, _name_figure);
  return fmt::format("{} {}.{}", _prefix, _name_chapter, _name_figure);
}

std::string pepp::Figure::description() const { return _description; }

void pepp::Figure::set_description(std::string description) { _description = description; }

bool pepp::Figure::is_problem() const { return _is_problem; }

bool pepp::Figure::is_os() const { return _is_os; }

void pepp::Figure::set_is_os(bool value) { _is_os = value; }

bool pepp::Figure::is_hidden() const { return _is_hidden; }

void pepp::Figure::set_is_hidden(bool value) { _is_hidden = value; }

const pepp::Figure *pepp::Figure::default_os() const { return _default_os; }

void pepp::Figure::set_default_os(const Figure *figure) { _default_os = figure; }

const std::list<const pepp::Test *> &pepp::Figure::tests() const { return _tests; }

void pepp::Figure::add_test(const Test *test) { _tests.push_back(test); }

const pepp::Fragment *pepp::Figure::find_fragment(const std::string &name) const {
  if (auto ret = _named_fragments.find(name); ret != _named_fragments.cend()) return ret->second;
  else return nullptr;
}

const std::vector<const pepp::Fragment *> &pepp::Figure::fragments() const { return _all_fragments; }

bool pepp::Figure::add_fragment(const pepp::Fragment *fragment) {
  const auto &name = fragment->name;
  // Unnamed fragments are always added. Such fragments are often used to produce exportable figures
  if (name.empty()) {
    _all_fragments.push_back(fragment);
    return true;
  }
  // Named fragments must be unique.
  else if (auto it = _named_fragments.find(name); it == _named_fragments.end()) {
    _all_fragments.push_back(fragment);
    _named_fragments[name] = fragment;
    return true;
  }
  SPDLOG_WARN("Duplicate fragment name '{}' in figure '{}'", name, name_full());
  return false;
}

std::string pepp::Figure::default_fragment_name() const { return _default_fragment_name; }

void pepp::Figure::set_default_fragment_name(std::string name) { _default_fragment_name = name; }

std::string pepp::Figure::default_fragment_text() const {
  if (_named_fragments.contains(_default_fragment_name)) return _named_fragments.at(_default_fragment_name)->contents();
  return {};
}

std::string pepp::Figure::default_os_text() const {
  if (_default_os) return _default_os->default_fragment_text();
  return {};
}
