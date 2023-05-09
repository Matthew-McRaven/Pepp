#include "fig.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include <iostream>

GetFigTask::GetFigTask(int ed, std::string ch, std::string fig, QObject *parent)
    : Task(parent), ed(ed), ch(ch), fig(fig) {}

void GetFigTask::run() {
  QString bookName;
  switch (ed) {
  case 6:
    bookName = "Computer Systems, 6th Edition";
  default:
    emit finished(1);
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);

  auto figure =
      book->findFigure(QString::fromStdString(ch), QString::fromStdString(fig));
  if (figure.isNull())
    return emit finished(1);
  if (!figure->typesafeElements().contains("pep"))
    return emit finished(2);

  auto body = figure->typesafeElements()["pep"]->contents;
  std::cout << body.toStdString() << std::endl;

  emit finished(0);
}
