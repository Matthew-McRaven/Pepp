/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include <QtCore>
#if INCLUDE_GUI
class QQmlApplicationEngine;
// Base class for application-specific global handling.
// Needed because an init_fn has a smaller lexical scope than the calling function,
// so stack-allocated "globals" fall out of scope.
struct gui_globals {
  virtual ~gui_globals() = default;
};
// Perform any type-registration, create & bind necessary globals.
using init_fn = QSharedPointer<gui_globals> (*)(QQmlApplicationEngine &);
#endif

namespace detail {
struct SharedFlags {
  // Somewhat of a tri-state value.
  // Record if we should explicitly use app default,
  // or if a subcommand has explicitly chosen a mode.
  enum class Kind { DEFAULT, TERM, GUI };
  Kind kind = Kind::DEFAULT;

  int edValue = 6;
};
} // namespace detail
