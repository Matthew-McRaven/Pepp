#include "./macro.hpp"
#include "../shared.hpp"
#include <iostream>

GetMacroTask::GetMacroTask(int ed, std::string name, QObject *parent)
    : Task(parent), ed(ed), name(name) {}

void GetMacroTask::run() {
  auto book = detail::book(ed);
  if (book.isNull())
    return emit finished(1);

  auto macro = book->findMacro(QString::fromStdString(name));
  if (macro.isNull())
    return emit finished(1);

  auto body = macro->body();
  std::cout << body.toStdString() << std::endl;

  return emit finished(0);
}
