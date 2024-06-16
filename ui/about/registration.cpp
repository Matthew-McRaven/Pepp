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

#include "./registration.hpp"
#include <QQmlEngine>
#include <iostream>
#include "contributors.hpp"
#include "dependencies.hpp"
#include "help/about/pepp.hpp"
#include "help/about/version.hpp"
#include "version.hpp"

namespace about {
void registerTypes(const char *uri) {
  qmlRegisterSingletonType<Version>(uri, 1, 0, "Version", [](QQmlEngine *, QJSEngine *) { return new Version(); });
  qmlRegisterUncreatableType<Maintainer>(uri, 1, 0, "Maintainer", "Must be created from C++");
  qmlRegisterSingletonType<QList<Maintainer *>>("edu.pepp", 1, 0, "Maintainers", [](QQmlEngine *, QJSEngine *) {
    // Need global scope ::, or it picks up about::Maintainer
    QList<::Maintainer *> maintainers{};
    for (const auto &maintainer : about::maintainers()) {
      auto *item = new ::Maintainer(maintainer.name, maintainer.email, nullptr);
      maintainers.push_back(item);
    }
    // Class assumes ownership of objects via modifying parent pointer.
    auto owning = new MaintainerList(maintainers);
    return owning;
  });
  qmlRegisterSingletonType<Contributors>(uri, 1, 0, "Contributors",
                                         [](QQmlEngine *, QJSEngine *) { return new Contributors(); });
  qmlRegisterUncreatableType<DependencyRoles>("edu.pepp", 1, 0, "DependencyRoles", "Error: only enums");
  qmlRegisterSingletonType<Dependencies>(uri, 1, 0, "Dependencies",
                                         [](QQmlEngine *, QJSEngine *) { return new Dependencies(); });
}

} // namespace about
