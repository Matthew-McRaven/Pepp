#include "ls.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include <iostream>

ListTask::ListTask(int ed, QObject *parent) : Task(parent), ed(ed) {}

void ListTask::run() {
  QString bookName;
  switch (ed) {
  case 6:
    bookName = "Computer Systems, 6th Edition";
  default:
    emit finished(1);
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);

  if (book.isNull())
    emit finished(1);

  auto figures = book->figures();
  auto macros = book->macros();

  std::cout << "Figures: " << std::endl;
  for (auto &figure : figures)
    std::cout << u"%1.%2"_qs.arg(figure->chapterName(), figure->figureName())
                     .toStdString()
              << std::endl;

  std::cout << std::endl;

  std::cout << "Macros: " << std::endl;
  for (auto &macro : macros)
    std::cout
        << u"%1 %2"_qs.arg(macro->name()).arg(macro->argCount()).toStdString()
        << std::endl;

  emit finished(0);
}
