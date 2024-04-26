#pragma once
#include "book_item_model.hpp"
#include "help/builtins/utils.hpp"

#include <QSortFilterProxyModel>

namespace builtins {
class BookFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(builtins::Architecture architecture READ architecture WRITE setArchitecture NOTIFY architectureChanged)
  Q_PROPERTY(builtins::Abstraction abstraction READ abstraction WRITE setAbstraction NOTIFY abstractionChanged)
  Q_PROPERTY(QAbstractItemModel *model READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
public:
  BookFilterModel(QObject *parent = nullptr);
  void setSourceModel(QAbstractItemModel *sourceModel) override;
  builtins::Architecture architecture() const { return _architecture; }
  void setArchitecture(builtins::Architecture architecture) {
    if (_architecture == architecture)
      return;
    _architecture = architecture;
    invalidateRowsFilter();
    emit architectureChanged();
  }
  builtins::Abstraction abstraction() const { return _abstraction; }
  void setAbstraction(builtins::Abstraction abstraction) {
    if (_abstraction == abstraction)
      return;
    _abstraction = abstraction;
    invalidateRowsFilter();
    emit abstractionChanged();
  }

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
signals:
  void sourceModelChanged();
  void architectureChanged();
  void abstractionChanged();

private:
  builtins::Architecture _architecture = builtins::Architecture::NONE;
  builtins::Abstraction _abstraction = builtins::Abstraction::NONE;

public:
};
} // namespace builtins
