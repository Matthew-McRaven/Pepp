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
  throw std::logic_error("Can't get here");
}
