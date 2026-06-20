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
#include "book.hpp"
#include "core/integers.h"
#include "figure.hpp"
#include "spdlog/spdlog.h"

pepp::Book::Book(std::string name) : _name(name) {}

std::string pepp::Book::name() const noexcept { return _name; }

pepp::Book::FigureVec &pepp::Book::figures() const noexcept { return _figures; }

namespace {
std::shared_ptr<const pepp::Figure> find_item(pepp::Book::FigureVec &vec, std::string chapter,
                                              std::string figure) noexcept {
  u64 count = 0;
  std::shared_ptr<const pepp::Figure> result = nullptr;
  for (const auto &ptr : vec) {
    if (ptr->name_chapter() == chapter && ptr->name_figure() == figure) {
      result = ptr;
      count++;
    }
  }
  if (count > 1) {
    spdlog::warn("More than one copy of figure {}.{} found", chapter, figure);
    return nullptr;
  } else return result;
}
} // namespace
std::shared_ptr<const pepp::Figure> pepp::Book::find_figure(std::string chapter, std::string figure) const noexcept {
  return find_item(_figures, chapter, figure);
}

bool pepp::Book::add_figure(std::shared_ptr<Figure> figure) noexcept {
  // TODO: Adding N figures will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for large N.
  if (find_figure(figure->name_chapter(), figure->name_figure()) == nullptr) {
    _figures.push_back(figure);
    return true;
  } else return false;
}

pepp::Book::FigureVec &pepp::Book::problems() const noexcept { return _problems; }

std::shared_ptr<const pepp::Figure> pepp::Book::find_problem(std::string chapter, std::string problem) const noexcept {
  return find_item(_problems, chapter, problem);
}

bool pepp::Book::add_problem(std::shared_ptr<Figure> problem) noexcept {
  // TODO: Adding N figures will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for problem N.
  if (find_problem(problem->name_chapter(), problem->name_figure()) == nullptr) {
    _problems.push_back(problem);
    return true;
  } else return false;
}

const std::vector<std::shared_ptr<pepp::MacroWrapper>> &pepp::Book::macros() const noexcept { return _macros; }

std::shared_ptr<const pepp::MacroWrapper> pepp::Book::find_macro(std::string name) const noexcept {
  u64 count = 0;
  std::shared_ptr<const MacroWrapper> result = nullptr;
  for (const auto &ptr : _macros) {
    if (ptr->definition->name == name) {
      result = ptr;
      count++;
    }
  }
  if (count > 1) {
    spdlog::warn("More than one copy of macro {} found", name);
    return nullptr;
  } else return result;
}

bool pepp::Book::add_macro(std::shared_ptr<MacroWrapper> macro) noexcept {
  // TODO: Adding N macros will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for large N.
  if (find_macro(macro->definition->name) == nullptr) {
    _macros.push_back(macro);
    return true;
  } else return false;
}
