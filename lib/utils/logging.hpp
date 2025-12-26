/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#include <functional>
#include <string>
#include "spdlog/spdlog.h"

// Helpers to allow lazy evaluation of lambdas that return strings for spdlog.
// This allows you to do expensive-ish computations for logging purposes but gate it behind logging levels.
// Derived from a post on reddit: https://www.reddit.com/r/cpp_questions/comments/oisjib/comment/h4xf40z/
template <typename F, typename... Args>
struct is_invocable : std::is_constructible<std::function<void(Args...)>,
                                            std::reference_wrapper<typename std::remove_reference<F>::type>> {};

template <typename R, typename F, typename... Args>
struct is_invocable_r : std::is_constructible<std::function<R(Args...)>,
                                              std::reference_wrapper<typename std::remove_reference<F>::type>> {};

template <typename T>
struct fmt::formatter<T, typename std::enable_if<is_invocable_r<std::string, T>::value, char>::type>
    : formatter<string_view> {
  template <typename FormatContext> auto format(T strFunc, FormatContext &ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(strFunc(), ctx);
  }
};
