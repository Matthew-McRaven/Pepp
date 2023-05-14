#include "fig.hpp"
#include "../shared.hpp"
#include "builtins/figure.hpp"
#include <iostream>

GetFigTask::GetFigTask(int ed, std::string ch, std::string fig, std::string type, QObject *parent)
    : Task(parent), ed(ed), ch(ch), fig(fig), type(type) {}

void GetFigTask::run() {
  auto book = detail::book(ed);
  if (book.isNull())
    return emit finished(1);

  auto figure =
      book->findFigure(QString::fromStdString(ch), QString::fromStdString(fig));
  static const auto err_nofig = u"Figure %1.%2 does not exist.\n"_qs;
  static const auto err_novar = u"Figure %1.%2 does not contain a \"%3\" variant.\n"_qs;
  if (figure.isNull()) {
    std::cerr << err_nofig.arg(QString::fromStdString(ch), QString::fromStdString(fig)).toStdString();
    return emit finished(1);
  }
  auto type = QString::fromStdString(this->type);
  if (!figure->typesafeElements().contains(type)) {
    std::cerr << err_novar.arg(QString::fromStdString(ch), QString::fromStdString(fig), type).toStdString();
    return emit finished(2);
  }

  auto body = figure->typesafeElements()[type]->contents;
  std::cout << body.toStdString() << std::endl;

  return emit finished(0);
}
