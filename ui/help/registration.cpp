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

#include "registration.hpp"
#include "book_item_model.hpp"
#include "bookfiltermodel.hpp"
#include "charcheck.hpp"

namespace help {
void registerTypes(const char *uri) {
  // TODO: Missing translations
  qmlRegisterType<builtins::BookModel>(uri, 1, 0, "BookModel");
  qmlRegisterType<builtins::BookFilterModel>(uri, 1, 0, "BookFilterModel");
  qmlRegisterType<CharCheck>(uri, 1, 0, "CharCheck");
}
} // namespace help
