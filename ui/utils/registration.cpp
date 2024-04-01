/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "registration.hpp"
#include <qqml.h>
#include "./constants.hpp"
#include "strings.hpp"

void utils::registerTypes(const char *uri) {
  qmlRegisterUncreatableType<utils::Abstraction>(uri, 1, 0, "Abstraction", error_only_enums);
  qmlRegisterUncreatableType<utils::Architecture>(uri, 1, 0, "Architecture", error_only_enums);
}
