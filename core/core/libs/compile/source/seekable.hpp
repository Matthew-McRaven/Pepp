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

#pragma once
#include <regex>
#include <string>
#include "./location.hpp"

namespace pepp::tc::support {
// A structure to support sequential reading of a 1d string and provide a row/column abstraction on top of it.
struct SeekableData {
  SeekableData() = default;
  explicit SeekableData(std::string&& d, support::Location loc = {0, 0});
  // View the next character without adjusting counters.
  char peek();
  // Get the text between _start and _end, inclusive;
  std::string_view select() const;
  // View the remaing text from _end to the end of data.
  std::string_view rest() const;
  // Returns true if there is more input to read.
  bool input_remains() const;

  // Advance the end and row cursors by n characters.
  void advance(size_t n);
  // Advance the start, end, and row cursors by n characters.
  void skip(size_t n);
  // Advance the row cursor by one, and reset the column cursor to 0.
  void newline();
  // Perform a AnchorAtOffsetMatchOption match at the current start position.
  // You must check the result for a match and call advance() if you want to move forward.
  using match = std::match_results<std::string_view::const_iterator>;
  match matchView(const std::regex &re);
  support::Location location() const;

private:
  std::string data = "\n";
  // Offsets into data
  size_t _start = 0, _end = 0;
  support::Location _loc;
};
} // namespace pepp::tc::support
