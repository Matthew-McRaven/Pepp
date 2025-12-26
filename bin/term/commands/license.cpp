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

#include "./license.hpp"
#include <iostream>
#include "catch.hpp"
#include "help/about/dependencies.hpp"

LicenseTask::LicenseTask(QObject *parent) : Task(parent) {}

void LicenseTask::run() {
  for (const auto &dependency : about::dependencies()) {
    std::cout << dependency.name.toStdString() << "\n";
    std::cout << "    URL: " << dependency.url.toStdString() << "\n";
    std::cout << "    License: " << dependency.licenseName.toStdString() << "\n";
    std::cout << "    License Text:\n";
    auto lines = Catch::TextFlow::Column(dependency.licenseText.toStdString());
    // License-injected newlines break formatting, so must manually substitute
    // in-place.
    for (const auto &line : lines)
      std::cout << "        " << QString::fromStdString(line).replace("\n", "\n        ").toStdString() << "\n";
    std::cout << "\n\n";
  }
  return emit finished(0);
}
