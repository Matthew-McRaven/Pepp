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
  auto macros = book->macros();

  std::cout << "Figures: " << std::endl;
  for (auto &figure : figures) {
    std::cout << u"%1.%2"_qs.arg(figure->chapterName(), figure->figureName()).leftJustified(10)
                     .toStdString();
    std::cout << figure->typesafeElements().keys().join(", ").toStdString();
    std::cout << std::endl;
    }

  std::cout << std::endl;

  if(macros.size() > 0) {
    std::cout << "Macros: " << std::endl;
    for (auto &macro : macros)
      std::cout
        << u"%1 %2"_qs.arg(macro->name()).arg(macro->argCount()).toStdString()
        << std::endl;
  }
  return emit finished(0);
}
