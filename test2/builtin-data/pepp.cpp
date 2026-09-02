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

#include "help/about/pepp.hpp"
#include <catch.hpp>

TEST_CASE("About Pepp", "[scope:help.about][kind:unit][arch:*]") {
  QDirIterator i(":/about", QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    if (!f.isFile()) continue;
    qDebug() << f.filePath();
  }

  CHECK_FALSE(about::projectRepoURL().size() == 0);
  CHECK_FALSE(about::maintainers().size() == 0);
  CHECK_FALSE(about::contributors().size() == 0);
  CHECK_FALSE(about::licenseFull().size() == 0);
  CHECK_FALSE(about::licenseNotice().size() == 0);
  CHECK(about::versionString().size() > 1);
}
