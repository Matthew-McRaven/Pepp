/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "fig.hpp"
#include <iostream>
#include "help/builtins/figure.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

GetFigTask::GetFigTask(int ed, std::string ch, std::string fig, std::string type, bool isFigure, QObject *parent)
    : Task(parent), ed(ed), isFigure(isFigure), ch(ch), fig(fig), type(type) {}

void GetFigTask::run() {
  using namespace Qt::StringLiterals;
  static const auto err_noitem = u"%1 %2.%3 does not exist.\n"_s;
  static const auto err_novar = u"%1 %2.%3 does not contain a \"%4\" variant.\n"_s;
  auto books = helpers::builtins_registry(false);
  auto book = helpers::book(ed, &*books);
  if (book.isNull())
    return emit finished(1);
  QSharedPointer<const builtins::Figure> item = nullptr;
  if (isFigure)
    item = book->findFigure(QString::fromStdString(ch), QString::fromStdString(fig));
  else
    item = book->findProblem(QString::fromStdString(ch), QString::fromStdString(fig));
  if (item.isNull()) {
    std::cerr << err_noitem.arg(isFigure ? "Figure" : "Problem")
                     .arg(QString::fromStdString(ch), QString::fromStdString(fig))
                     .toStdString();
    return emit finished(1);
  }

  auto type = QString::fromStdString(this->type);
  if (!item->typesafeNamedFragments().contains(type)) {
    std::cerr << err_novar.arg(isFigure ? "Figure" : "Problem")
                     .arg(QString::fromStdString(ch), QString::fromStdString(fig), type)
                     .toStdString();
    return emit finished(2);
  }

  auto body = item->typesafeNamedFragments()[type]->contents();
  std::cout << body.toStdString() << std::endl;

  return emit finished(0);
}
