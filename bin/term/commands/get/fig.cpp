/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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
#include "core/resources/figures/book.hpp"
#include "fmt/format.h"
#include "toolchain/helpers/assemblerregistry.hpp"

GetFigTask::GetFigTask(int ed, std::string ch, std::string fig, std::string type, bool isFigure, QObject *parent)
    : Task(parent), ed(ed), isFigure(isFigure), ch(ch), fig(fig), type(type) {}

void GetFigTask::run() {

  auto books = helpers::builtins_registry(false);
  auto book = helpers::book(ed, &*books);
  if (book == nullptr) return emit finished(1);
  std::shared_ptr<const pepp::Figure> item = nullptr;
  if (isFigure) item = book->find_figure(ch, fig);
  else item = book->find_problem(ch, fig);
  if (item == nullptr) {
    std::cerr << fmt::format("{} {}.{} does not exist.\n", isFigure ? "Figure" : "Problem", ch, fig);
    return emit finished(1);
  }

  if (!item->has_fragment(this->type)) {
    std::cerr << fmt::format("{} {}.{} does not contain a \"{}\" variant.\n", isFigure ? "Figure" : "Problem", ch, fig,
                             type);

    return emit finished(2);
  }

  auto body = item->find_fragment(type)->contents();
  std::cout << body << std::endl;

  return emit finished(0);
}
