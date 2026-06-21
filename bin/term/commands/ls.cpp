/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "ls.hpp"
#include <iostream>
#include <ranges>
#include "../shared.hpp"
#include "fmt/ranges.h"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

ListTask::ListTask(int ed, QObject *parent) : Task(parent), ed(ed) {}

void ListTask::run() {
  using namespace Qt::StringLiterals;
  auto books = helpers::builtins_registry(false);
  auto book = helpers::book(ed, &*books);
  if (book == nullptr) return emit finished(1);
  auto figures = book->figures();
  auto problems = book->problems();
  auto macros = book->macros();

  // Prevent figure name from overlapping with file types. See #305.
  int maxFigWidth = 10;
  for (auto &figure : figures)
    maxFigWidth =
        std::max<int>(figure->name_chapter().length() + 1 /*.*/ + figure->name_figure().length(), maxFigWidth);
  for (auto &problem : problems)
    maxFigWidth =
        std::max<int>(problem->name_chapter().length() + 1 /*.*/ + problem->name_figure().length(), maxFigWidth);

  int maxMacroWidth = 6;
  for (auto macro : macros) maxMacroWidth = std::max<int>(macro->name.length(), maxMacroWidth);
  if (this->showFigs) {
    std::cout << "Figures: " << std::endl;
    for (auto &figure : figures) {
      std::cout
          << u"%1.%2"_s.arg(figure->name_chapter(), figure->name_figure()).leftJustified(maxFigWidth + 2).toStdString();
      if (this->showTestCount) std::cout << u"%1"_s.arg(figure->tests().size(), -4).toStdString();
      std::cout << fmt::format("{}", fmt::join(figure->named_fragments() | std::views::keys, ", "));
      std::cout << std::endl;
    }
  }

  if (problems.size() > 0 && this->showProbs) {
    std::cout << "\nProblems: \n";
    for (auto &problem : problems) {
      std::cout << u"%1.%2"_s.arg(problem->name_chapter(), problem->name_figure())
                       .leftJustified(maxFigWidth + 2)
                       .toStdString();
      if (this->showTestCount) std::cout << u"%1"_s.arg(problem->tests().size(), -4).toStdString();
      std::cout << fmt::format("{}", fmt::join(problem->named_fragments() | std::views::keys, ", "));
      std::cout << std::endl;
    }
  }

  if (macros.size() > 0 && this->showMacros) {
    std::cout << "\nMacros: \n";
    for (auto &macro : macros)
      std::cout << u"%1"_s.arg(macro->name).leftJustified(maxMacroWidth + 2).toStdString()
                << u"%1"_s.arg(macro->argcount).toStdString() << std::endl;
  }
  return emit finished(0);
}
