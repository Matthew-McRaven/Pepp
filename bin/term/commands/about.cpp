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

#include "./about.hpp"
#include <iostream>
#include <catch.hpp>
#include "help/about/dependencies.hpp"
#include "help/about/pepp.hpp"
#include "help/about/version.hpp"

AboutTask::AboutTask(QObject *parent) : Task(parent) {}

void AboutTask::run() {
  using namespace Qt::StringLiterals;
  static const auto build_system = QStringLiteral("%1 %2 %3")
                                       .arg(about::g_BUILD_SYSTEM_NAME())
                                       .arg(about::g_BUILD_SYSTEM_VERSION())
                                       .arg(about::g_BUILD_SYSTEM_PROCESSOR());
  static const auto compiler_id =
      QStringLiteral("%1 %2").arg(about::g_CXX_COMPILER_ID()).arg(about::g_CXX_COMPILER_VERSION());

  std::cout << u"Pepp Terminal, Version %1"_s.arg(about::versionString()).toStdString();
  for (const auto &line : about::diagnostics()) std::cout << "\n\t" << line.toStdString();
  std::cout << "\n\nReport issues or check for updates:\n";
  std::cout << "\t" << about::projectRepoURL().toStdString() << "\n\n";
  std::cout << "Authors:\n";
  for (const auto &maintainer : about::maintainers())
    std::cout << "\t" << maintainer.name.toStdString() << " <" << maintainer.email.toStdString() << "> \n";
  std::cout << "\nLicensing:\n";
  auto lines = Catch::TextFlow::Column(about::licenseNotice().toStdString());
  lines.width(72);
  for (const auto &line : lines)
    std::cout << "\t" << QString::fromStdString(line).replace("\n", "\n\t").toStdString() << "\n";
  std::cout << "\n";
  lines = Catch::TextFlow::Column("For further licensing info, execute this program with the `license` subcommand.\n");
  lines.width(72);
  for (const auto &line : lines)
    std::cout << "\t" << QString::fromStdString(line).replace("\n", "\n\t").toStdString() << "\n";
  return emit finished(0);
}
