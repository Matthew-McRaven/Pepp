#include "bookfiltermodel.hpp"
#include "help/builtins/utils.hpp"

builtins::BookFilterModel::BookFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void builtins::BookFilterModel::setSourceModel(QAbstractItemModel *sourceModel) {
  if (sourceModel == this->sourceModel())
    return;
  QSortFilterProxyModel::setSourceModel(sourceModel);
  emit sourceModelChanged();
}

bool builtins::BookFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  using builtins::FigureConstants;
  auto sm = sourceModel();
  if (!sm) return false;
  else if (_architecture == builtins::Architecture::NONE) return true;
  auto index = sm->index(source_row, 0, source_parent);
  auto kind = sm->data(index, FigureConstants::FIG_ROLE_KIND).toString();
  if (kind == "book") {
    auto title = sm->data(index, FigureConstants::FIG_ROLE_EDITION).toString();
    if (title == "Computer Systems, 6th Edition" &&
        (_architecture == builtins::Architecture::PEP10 || _architecture == builtins::Architecture::RISCV))
      return true;
    else if (title == "Computer Systems, 5th Edition" && _architecture == builtins::Architecture::PEP9) return true;
    else if (title == "Computer Systems, 4th Edition" && _architecture == builtins::Architecture::PEP8) return true;
    return false;
  } else if (kind == "figure") {
    if (_abstraction == builtins::Abstraction::NONE) return true;
    auto architecture = sm->data(index, FigureConstants::FIG_ROLE_ARCHITECTURE).value<builtins::Architecture>();
    auto abstraction = sm->data(index, FigureConstants::FIG_ROLE_ABSTRACTION).value<builtins::Abstraction>();
    return architecture == _architecture && abstraction == _abstraction;
  }
  return true;
}
