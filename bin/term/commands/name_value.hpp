/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <charconv>
#include <concepts>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace detail {
// Parses all of text in the given base, rejecting empty input, trailing characters, and out-of-range values.
template <typename T> std::optional<T> parse_whole(std::string_view text, int base) {
  T value{};
  const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value, base);
  if (text.empty() || ec != std::errc{} || end != text.data() + text.size()) return std::nullopt;
  return value;
}
} // namespace detail

// Parses <name>=<value>, where value is a signed decimal, unsigned decimal, or 0x-prefixed hex integer that fits in U.
// Negative values are kept as their two's complement.
template <std::unsigned_integral U> std::optional<std::pair<std::string, U>> parse_name_value(std::string_view arg) {
  const auto eq = arg.find('=');
  if (eq == 0 || eq == std::string_view::npos) return std::nullopt;
  const auto name = arg.substr(0, eq), text = arg.substr(eq + 1);
  std::optional<U> value;
  if (text.starts_with("0x") || text.starts_with("0X")) value = detail::parse_whole<U>(text.substr(2), 16);
  else if (text.starts_with('-')) {
    if (const auto negative = detail::parse_whole<std::make_signed_t<U>>(text, 10)) value = static_cast<U>(*negative);
  } else value = detail::parse_whole<U>(text, 10);
  if (!value) return std::nullopt;
  return std::make_pair(std::string(name), *value);
}
