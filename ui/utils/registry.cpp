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

#include "registry.hpp"

#include <stdexcept>

std::shared_ptr<utils::QMLTypeRegistry> utils::QMLTypeRegistry::instance() {
  struct SharedHelper : public utils::QMLTypeRegistry {
    SharedHelper() : QMLTypeRegistry(){};
  };
  static std::shared_ptr<utils::QMLTypeRegistry> instance = std::make_shared<SharedHelper>();
  return instance;
}

void utils::QMLTypeRegistry::addRegistration(registration reg) { _registrations.push_back(std::move(reg)); }

void utils::QMLTypeRegistry::doRegistrations(const char *uri) {
  static bool initialized = false;
  if (initialized)
    throw std::logic_error("Attempted to re-register QML types.");
  for (auto reg : _registrations)
    reg(uri);
  initialized = true;
}

utils::QMLTypeRegistry::QMLTypeRegistry() = default;

utils::RegistrationHelper::RegistrationHelper(registration reg) {
  QMLTypeRegistry::instance()->addRegistration(std::move(reg));
}
