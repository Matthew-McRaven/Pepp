#include "ls.hpp"
#include "./shared.hpp"
#include "builtins/figure.hpp"
#include <iostream>

ListTask::ListTask(int ed, QObject *parent) : Task(parent), ed(ed) {}

void ListTask::run() {
  auto book = detail::book(ed);
  if (book.isNull())
    return emit finished(1);
  auto figures = book->figures();
  auto problems = book->problems();
  auto macros = book->macros();

  // Prevent figure name from overlapping with file types. See #305.
  int maxFigWidth = 10;
  for (auto figure : figures)
    maxFigWidth = std::max<int>(figure->chapterName().length() + 1 /*.*/ +
                                    figure->figureName().length(),
                                maxFigWidth);
  for (auto problem : problems)
    maxFigWidth = std::max<int>(problem->chapterName().length() + 1 /*.*/ +
                                    problem->figureName().length(),
                                maxFigWidth);

  int maxMacroWidth = 6;
  for (auto macro : macros)
    maxMacroWidth = std::max<int>(macro->name().length(), maxMacroWidth);
  std::cout << "Figures: " << std::endl;
  for (auto &figure : figures) {
    std::cout << u"%1.%2"_qs.arg(figure->chapterName(), figure->figureName())
                     .leftJustified(maxFigWidth + 2)
                     .toStdString();
    std::cout << figure->typesafeElements().keys().join(", ").toStdString();
    std::cout << std::endl;
  }

  if (problems.size() > 0) {
    std::cout << "\nProblems: \n";
    for (auto &problem : problems) {
      std::cout << u"%1.%2"_qs
                       .arg(problem->chapterName(), problem->figureName())
                       .leftJustified(maxFigWidth + 2)
                       .toStdString();
      std::cout << problem->typesafeElements().keys().join(", ").toStdString();
      std::cout << std::endl;
    }
  }

  if (macros.size() > 0) {
    std::cout << "\nMacros: \n";
    for (auto &macro : macros)
      std::cout << u"%1"_qs.arg(macro->name())
                       .leftJustified(maxMacroWidth + 2)
                       .toStdString()
                << u"%1"_qs.arg(macro->argCount()).toStdString() << std::endl;
  }
  return emit finished(0);
}
