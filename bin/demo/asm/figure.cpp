/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include "./figure.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include <QQmlEngine>
FigureManager::FigureManager() {
  auto registry = _reg = QSharedPointer<builtins::Registry>::create(nullptr);
  auto book = registry->findBook("Computer Systems, 6th Edition");
  auto figures = book->figures();
  for (auto &figure : figures) {
    if (figure->isOS())
      continue;
    else if (!figure->typesafeElements().contains("pep"))
      continue;
    _figureMap[_figureMap.size()] = figure;
  }
}

QStringList FigureManager::figures() {
  QStringList ret;

  auto keys = _figureMap.keys();
  std::sort(keys.begin(), keys.end());

  for (auto key : keys) {
    auto fig = _figureMap[key];
    auto chName = fig->chapterName();
    auto figName = fig->figureName();
    ret.push_back(u"Figure %1.%2"_qs.arg(chName, figName));
  }

  return ret;
}

builtins::Figure *FigureManager::figureAt(qsizetype index) {
  if (auto it = _figureMap.find(index); it != _figureMap.end()) {
    auto ptr = &**it;
    QQmlEngine::setObjectOwnership(ptr, QQmlEngine::CppOwnership);
    return ptr;
  }
  static const char *const e = "Unreachable";
  qCritical(e);
  throw std::logic_error(e);
}
