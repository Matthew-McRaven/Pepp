#include "fig.hpp"
#include "../shared.hpp"
#include "builtins/figure.hpp"
#include <iostream>

GetFigTask::GetFigTask(int ed, std::string ch, std::string fig,
                       std::string type, bool isFigure, QObject *parent)
    : Task(parent), ed(ed), isFigure(isFigure), ch(ch), fig(fig), type(type) {}

void GetFigTask::run() {
  static const auto err_noitem = u"%1 %2.%3 does not exist.\n"_qs;
  static const auto err_novar =
      u"%1 %2.%3 does not contain a \"%4\" variant.\n"_qs;

  auto book = detail::book(ed);
  if (book.isNull())
    return emit finished(1);
  QSharedPointer<const builtins::Figure> item = nullptr;
  if (isFigure)
    item = book->findFigure(QString::fromStdString(ch),
                            QString::fromStdString(fig));
  else
    item = book->findProblem(QString::fromStdString(ch),
                             QString::fromStdString(fig));
  if (item.isNull()) {
    std::cerr << err_noitem.arg(isFigure ? "Figure" : "Problem")
                     .arg(QString::fromStdString(ch),
                          QString::fromStdString(fig))
                     .toStdString();
    return emit finished(1);
  }

  auto type = QString::fromStdString(this->type);
  if (!item->typesafeElements().contains(type)) {
    std::cerr << err_novar.arg(isFigure ? "Figure" : "Problem")
                     .arg(QString::fromStdString(ch),
                          QString::fromStdString(fig), type)
                     .toStdString();
    return emit finished(2);
  }

  auto body = item->typesafeElements()[type]->contents;
  std::cout << body.toStdString() << std::endl;

  return emit finished(0);
}
