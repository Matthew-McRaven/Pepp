#pragma once
#include <QAbstractListModel>
#include <QHash>

namespace ELFIO {
class elfio;
}
class SymbolModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(qsizetype longest MEMBER longest_ CONSTANT)

  struct Entry {
    QString name;
    uint64_t value;
  };
  QList<Entry> entries_;
  QHash<int, QByteArray> roleNames_;
  qsizetype longest_{0};

public:
  // Define the role names to be used
  enum RoleNames : quint32 {
    SymbolRole = Qt::UserRole,
    ValueRole,
    IndexRole,
  };
  Q_ENUM(RoleNames)

  SymbolModel(QObject *parent = nullptr);
  void setFromElf(ELFIO::elfio *elf, QString tableSection);
  void clearData();
  // QAbstractItemModel interface
  int rowCount(const QModelIndex &parent) const override;
  // int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;

protected: //  Role Names must be under protected
  QHash<int, QByteArray> roleNames() const override;
};
