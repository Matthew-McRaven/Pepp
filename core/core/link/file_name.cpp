/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include "core/link/file_name.hpp"

std::string pepp::link::input_file_name(const decltype(LinkInput::source) &source) {
  if (const auto *path = std::get_if<std::filesystem::path>(&source)) return path->string();
  else if (const auto *opened = std::get_if<LinkInput::Opened>(&source)) return opened->path.string();
  else if (const auto *script = std::get_if<LinkInput::Script>(&source))
    if (const auto *path = std::get_if<std::filesystem::path>(&script->source)) return path->string();
  return "<memory>";
}