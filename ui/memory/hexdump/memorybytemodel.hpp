#ifndef MEMORYBYTEMODEL_H
#define MEMORYBYTEMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QSet>
#include "../memory_globals.hpp"
#include "memorycolumns.hpp"
#include "rawmemory.hpp"
#include "utils/opcodemodel.hpp"

class MEMORY_EXPORT MemoryRoles : public QObject {
  Q_OBJECT
public:
  enum Roles {
    Selected = Qt::UserRole + 1,
    Highlight,
    Editing,
    Type,
  };
  Q_ENUM(Roles)
};

class MEMORY_EXPORT MemoryByteModel : public QAbstractTableModel {
  Q_OBJECT

  //  Statistics on memory size and layout
  quint8 width_ = 8; //  Default to 8 columns
  quint32 height() const;
  EmptyRawMemory *empty_ = nullptr;
  bool reclaimMemory_ = false;
  ARawMemory *memory_ = nullptr;
  OpcodeModel *mnemonics_ = nullptr;
  QSet<quint8> selected_;
  qint32 editing_ = -1;
  qint32 lastEdit_ = -1;
  std::unique_ptr<MemoryColumns> column_;

  Q_PROPERTY(MemoryColumns *Column READ column CONSTANT)
  Q_PROPERTY(ARawMemory *memory READ memory WRITE setMemory NOTIFY memoryChanged)
  Q_PROPERTY(OpcodeModel *mnemonics MEMBER mnemonics_ NOTIFY mnemonicsChanged)
  Q_PROPERTY(int bytesPerRow READ bytesPerRow NOTIFY dimensionsChanged)
  // Q_PROPERTY(int columns READ columnCount NOTIFY dimensionsChanged)
  // Q_PROPERTY(int rows READ rowCount NOTIFY dimensionsChanged)

public:
  // Define the role names to be used

  explicit MemoryByteModel(QObject *parent = nullptr, const quint8 bytesPerRow = 8);
  ~MemoryByteModel() = default;

  //  Required for access in Qml
  MemoryColumns *column() { return column_.get(); }

  ARawMemory *memory() const;
  void setMemory(ARawMemory *memory);

  //  Helper functions for other C++ functions to call and affect the
  //  memory model without knowledge of the model layout.
  quint8 readByte(quint32 address) const;
  void writeByte(quint32 address, quint8 value);

  // Set the number of bytes displayed per line.
  // Should be a power of 2 between [1-32].
  // A number that is not a power of two will be rounded to the nearest power,
  // and the number will be clamped to 32.
  void setNumBytesPerLine(const quint8 bytesPerLine);

  //  Functions required by the QML model
  //  Header: Set read column and row headers
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  //  Basic functionality: dimentions of table in QML view
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  int bytesPerRow() const;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  //  Custom functions available to Qml
  Q_INVOKABLE bool isEditMode() { return (editing_ != -1); };

  //  Functions are based on index in Qml.
  Q_INVOKABLE void clearSelected(const QModelIndex &index, const MemoryRoles::Roles type = MemoryRoles::Selected);
  Q_INVOKABLE QVariant setSelected(const QModelIndex &index, const MemoryRoles::Roles type = MemoryRoles::Selected);

  Q_INVOKABLE QModelIndex currentCell();
  Q_INVOKABLE QModelIndex lastCell();
public slots:
  void onDataChanged(quint32 start, quint32 end);
signals:
  void dimensionsChanged();
  void memoryChanged();
  void mnemonicsChanged();

protected: //  Role Names must be under protected
  //  Columns available to Qml model. Represents a translation from
  //  Name in bytearray and RoleNames enum. Enum is used as role in
  //  callbacks to model.
  QHash<int, QByteArray> roleNames() const override;

private:
  //  Conversion functions
  std::size_t memoryOffset(const QModelIndex &index) const;
  QModelIndex memoryIndex(std::size_t index);

  //  Return ascii text for row
  QString ascii(const int row) const;

  //  Clear model
  void clear();

  //  Private functions fo manipulate selection directly by model
  QVariant selected(const QModelIndex &index, const MemoryRoles::Roles type = MemoryRoles::Selected) const;
  // QVariant setSelected(const QModelIndex& index,const RoleNames type = Selected);
  // void clearSelected(const QModelIndex& index, const RoleNames type = Selected);

  // Likely to be deleted
  void clearSelectedCell(const int offset, const MemoryRoles::Roles type = MemoryRoles::Selected);
};

#endif // MEMORYBYTEMODEL_H
