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
#include "contributors.hpp"
#include "help/about/pepp.hpp"
#include "help/about/version.hpp"
#include "projects.hpp"
#include "version.hpp"

namespace about {
void registerTypes(QQmlApplicationEngine &engine) {
  qmlRegisterSingletonType<Version>("edu.pepp", 1, 0, "Version",
                                    [](QQmlEngine *, QJSEngine *) { return new Version(); });
  qmlRegisterUncreatableType<Maintainer>("edu.pepp", 1, 0, "Maintainer", "Must be created from C++");
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
  qmlRegisterSingletonType<Contributors>("edu.pepp", 1, 0, "Contributors",
                                         [](QQmlEngine *, QJSEngine *) { return new Contributors(); });
  qmlRegisterUncreatableType<ProjectRoles>("edu.pepp", 1, 0, "ProjectRoles", "Error: only enums");
  qmlRegisterSingletonType<Projects>("edu.pepp", 1, 0, "Projects",
                                     [](QQmlEngine *, QJSEngine *) { return new Projects(); });
}
} // namespace about
