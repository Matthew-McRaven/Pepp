/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "./seekable.hpp"

pepp::tc::support::SeekableData::SeekableData(std::string &&d, support::Location loc) : data(std::move(d)), _loc(loc) {}

char pepp::tc::support::SeekableData::peek() {
  if (!input_remains()) return '\0';
  return data[_end];
}

std::string_view pepp::tc::support::SeekableData::select() const {
  auto count = _end - _start;
  return data.substr(_start, count);
}

std::string_view pepp::tc::support::SeekableData::rest() const { return data.substr(_end); }

bool pepp::tc::support::SeekableData::input_remains() const { return _end < data.size(); }

void pepp::tc::support::SeekableData::advance(size_t n) {
  _end += n;
  _loc.column += n;
}

void pepp::tc::support::SeekableData::skip(size_t n) {
  _start = _end += n;
  _loc.column += n;
}

void pepp::tc::support::SeekableData::newline() { _loc.row++, _loc.column = 0; }

pepp::tc::support::SeekableData::match pepp::tc::support::SeekableData::matchView(const std::regex &re) {
  auto str = std::string_view{data}.substr(_start);

  // match_continuous acts like old Anchored.
  std::match_results<std::string_view::const_iterator> m;
  std::regex_search(str.begin(), str.end(), m, re, std::regex_constants::match_continuous);
  return m;
}

pepp::tc::support::Location pepp::tc::support::SeekableData::location() const { return _loc; }
