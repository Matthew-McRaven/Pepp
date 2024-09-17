#pragma once
#include <QAbstractTableModel>

namespace ELFIO {
class elfio;
}
class SymbolModel : public QAbstractTableModel {
  Q_OBJECT
public:
  SymbolModel(QObject *parent = nullptr);
  void setFromElf(ELFIO::elfio *elf, QString tableSection);
  void clearData();
  // QAbstractItemModel interface
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;

private:
  struct Entry {
    QString name;
    uint64_t value;
  };
  QList<Entry> _entries;
};
