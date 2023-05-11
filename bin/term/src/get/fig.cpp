#include "fig.hpp"
#include "../shared.hpp"
#include "builtins/figure.hpp"
#include <iostream>

GetFigTask::GetFigTask(int ed, std::string ch, std::string fig, QObject *parent)
    : Task(parent), ed(ed), ch(ch), fig(fig) {}

void GetFigTask::run() {
  auto book = detail::book(ed);
  if (book.isNull())
    return emit finished(1);

  auto figure =
      book->findFigure(QString::fromStdString(ch), QString::fromStdString(fig));
  if (figure.isNull())
    return emit finished(1);
  if (!figure->typesafeElements().contains("pep"))
    return emit finished(2);

  auto body = figure->typesafeElements()["pep"]->contents;
  std::cout << body.toStdString() << std::endl;

  return emit finished(0);
}
