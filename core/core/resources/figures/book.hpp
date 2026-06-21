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

#include <memory>
#include <string>
#include <vector>
#include "core/architectures.hpp"

namespace pepp {
// TODO: when we port to the new assembler, all of these fields will change.
// The file on the disk might contain multiple macro defs, so to actually know what macros a file defines you have to
// parse it. Because Pepp still uses the old assembler, we still have to support files starting with the `@MACRO_NAME #`
// format.
struct MacroFile {
  std::string name;
  std::string body;
  u8 argcount;
  // Meta-information used to filter & sort macros within the book.
  std::string family;
  pepp::Architecture arch = pepp::Architecture::NO_ARCH;
  bool hidden = false;
};

class Figure;
class Book {
public:
  using FigureVec = const std::vector<std::shared_ptr<Figure>> &;
  explicit Book(std::string name);
  // Return the full name of the textbook.
  std::string name() const noexcept;
  // Return a container of all figures within this book.
  FigureVec &figures() const noexcept;
  // Return a pointer to the figure with the given name, or a nullptr if no such figure exists.
  std::shared_ptr<const Figure> find_figure(std::string chapter, std::string figure) const noexcept;
  // Register a figure as part of this book, returning false if a figure with the same name already exists.
  bool add_figure(std::shared_ptr<Figure> figure) noexcept;

  // Return a container of all problems within this book.
  FigureVec &problems() const noexcept;
  // Return a pointer to the problem with the given name, or a nullptr if no such problem exists.
  std::shared_ptr<const Figure> find_problem(std::string chapter, std::string problem) const noexcept;
  // Register a problem as part of this book, returning false if a problem with the same name already exists.
  bool add_problem(std::shared_ptr<Figure> problem) noexcept;

  // Return a container of all macros within this book.
  const std::vector<std::shared_ptr<MacroFile>> &macros() const noexcept;
  // Return a pointer to the macro with the given name, or a nullptr if no such macro exists.
  // We do not allow multiple  macros with the same name.
  std::shared_ptr<const MacroFile> find_macro(std::string name) const noexcept;
  // Register a macro as part of this book, returning false if a macro with the same name already exists.
  bool add_macro(std::shared_ptr<MacroFile> macro) noexcept;

private:
  std::string _name;
  std::vector<std::shared_ptr<Figure>> _figures = {}, _problems = {};
  std::vector<std::shared_ptr<MacroFile>> _macros = {};
};
} // namespace pepp
