#pragma once
#include <QAbstractListModel>
#include <QHash>

namespace ELFIO {
class elfio;
}
class QItemSelectionModel;
class SymbolModel : public QAbstractTableModel {
  Q_OBJECT

  Q_PROPERTY(qsizetype longest READ longest NOTIFY longestChanged)

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
  int columnCount(const QModelIndex &parent) const override;
  Q_INVOKABLE void setColumnCount(int count);
  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  // Helper method that is only here because I don't want another global helper class
  Q_INVOKABLE void selectRectangle(QItemSelectionModel *selectionModel, const QModelIndex &topLeft,
                                   const QModelIndex &bottomRight) const;

  qsizetype longest() const { return longest_; }

signals:
  void longestChanged();

protected:
  QHash<int, QByteArray> roleNames() const override;

private:
  int _columnCount{2};
};
