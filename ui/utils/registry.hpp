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

#pragma once
#include <functional>
#include <memory>
#include "utils_global.hpp"

namespace utils {
using registration = std::function<void(const char *)>;
// Only create instances with static lifetime, or with regs that have a static lifetime.
class UTILS_EXPORT RegistrationHelper {
public:
  RegistrationHelper(registration reg);
};

// Singleton which can contains functions that perform QML type registration.
class UTILS_EXPORT QMLTypeRegistry {
public:
  static std::shared_ptr<QMLTypeRegistry> instance();
  // Will not de-duplicate registration functions.
  void addRegistration(registration reg);
  void doRegistrations(const char *uri);

private:
  std::list<registration> _registrations = {};
  QMLTypeRegistry();
};
} // namespace utils
