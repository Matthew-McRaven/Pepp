#include "io.hpp"
#include <iostream>
#include "help/builtins/figure.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

GetIOTask::GetIOTask(int ed, std::string ch, std::string fig, bool isFigure, int testIdx, QObject *parent)
    : Task(parent), ed(ed), testIdx(testIdx), isFigure(isFigure), ch(ch), fig(fig) {}

void GetIOTask::run() {
  using namespace Qt::StringLiterals;
  static const auto err_noitem = u"%1 %2.%3 does not exist.\n"_s;
  auto books = helpers::builtins_registry(false);
  auto book = helpers::book(ed, &*books);
  if (book.isNull()) return emit finished(1);
  QSharedPointer<const builtins::Figure> item = nullptr;
  if (isFigure) item = book->findFigure(QString::fromStdString(ch), QString::fromStdString(fig));
  else item = book->findProblem(QString::fromStdString(ch), QString::fromStdString(fig));
  if (item.isNull()) {
    std::cerr << err_noitem.arg(isFigure ? "Figure" : "Problem")
                     .arg(QString::fromStdString(ch), QString::fromStdString(fig))
                     .toStdString();
    return emit finished(1);
  }
  auto tests = item->typesafeTests();
  if (testIdx < 0 || testIdx >= tests.size()) {
    std::cerr << "Invalid test index: " << testIdx << ". Valid range is 0 to " << (tests.size() - 1) << ".\n";
    return emit finished(2);
  }
  auto test = tests[testIdx];
  std::cout << test->input.toString().trimmed().toStdString() << std::endl;

  return emit finished(0);
}
