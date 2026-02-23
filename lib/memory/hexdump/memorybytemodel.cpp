/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#include "memorybytemodel.hpp"
#include <QBrush>
#include <QColor>
#include <QPoint>
#include <QQmlEngine>
#include <QRect>
#include "core/math/bitmanip/log2.hpp"
#include "rawmemory.hpp"
#include "settings/settings.hpp"

using namespace Qt::StringLiterals;

quint32 MemoryByteModel::height() const {
  //  Compute memory height.
  const auto size = memory_->byteCount();
  auto height = size / width_;

  //  Pad last row if not exactly divisible
  if ((size % width_) != 0) ++height;
  return height;
}

MemoryByteModel::MemoryByteModel(QObject *parent, const quint8 bytesPerRow)
    : QAbstractTableModel(parent), empty_(new EmptyRawMemory(0, this)), memory_(empty_), column_(new MemoryColumns) {
  //  Changing width also changes height
  reclaimMemory_ = false;
  QQmlEngine::setObjectOwnership(empty_, QQmlEngine::CppOwnership);
  QQmlEngine::setObjectOwnership(memory_, QQmlEngine::CppOwnership);
  setNumBytesPerLine(bytesPerRow);
  clear();
}

ARawMemory *MemoryByteModel::memory() const {
  QQmlEngine::setObjectOwnership(memory_, QQmlEngine::CppOwnership);
  return memory_;
}

void MemoryByteModel::setMemory(ARawMemory *memory) {
  if (memory_ == memory) return;

  beginResetModel();
  if (reclaimMemory_) delete memory_;
  else
    // Delete would disconnect. If the memory remains, we must not respond to its signals.
    disconnect(memory_, &ARawMemory::dataChanged, this, &MemoryByteModel::onDataChanged);

  if (memory == nullptr) memory_ = empty_;
  else memory_ = memory;
  connect(memory_, &ARawMemory::dataChanged, this, &MemoryByteModel::onDataChanged);
  // We only need to reclaim the "memory" pointer if it is currently owned by JS.
  // If it is alreay owned by CPP, then it is either empty_ or came from an project.
  // In either case, we should not delete it.
  reclaimMemory_ = QQmlEngine::objectOwnership(memory_) == QQmlEngine::JavaScriptOwnership;
  // Must take ownership of data or crashes may occur.
  // Because of ownership changes, we must delete memory_ when we replace it.
  QQmlEngine::setObjectOwnership(memory_, QQmlEngine::CppOwnership);
  emit memoryChanged();
  endResetModel();
}

void MemoryByteModel::setMemory(QObject *memory) {
  auto mem = qobject_cast<ARawMemory *>(memory);
  if (mem) setMemory(mem);
}

OpcodeModel *MemoryByteModel::mnemonics() const { return mnemonics_; }

void MemoryByteModel::setMnemonics(OpcodeModel *mn) {
  if (mnemonics_ == mn) return;
  QQmlEngine::setObjectOwnership(mn, QQmlEngine::CppOwnership);
  mnemonics_ = mn;
  emit mnemonicsChanged();
}

void MemoryByteModel::setMnemonics(QObject *mn) {
  auto model = qobject_cast<OpcodeModel *>(mn);
  if (model) setMnemonics(model);
}

quint8 MemoryByteModel::readByte(const quint32 address) const { return memory_->read(address); }

void MemoryByteModel::writeByte(const quint32 address, const quint8 value) {
  memory_->write(address, value);

  const auto index = memoryIndex(address);

  //  Write byte to model
  setData(index, value, Qt::DisplayRole);
}

void MemoryByteModel::setNumBytesPerLine(quint8 bytesPerLine) {
  Q_ASSERT(bytesPerLine > 0);
  // Round up bytesPerLine to nearest power of 2.
  bytesPerLine = bits::nearest_power_of_two(bytesPerLine);
  if (bytesPerLine == width_) return;
  beginResetModel();
  //  Set bytes per row
  //  Initialized on construction to 8 bytes per row. If values are invalid, default is used
  if (bytesPerLine == 0) width_ = 8;
  else {
    //  Limit size to 32 since screen refresh will be slow
    width_ = bytesPerLine > 32 ? 32 : bytesPerLine;
  }

  //  Updated column identifiers for width change
  column_->setNumBytesPerLine(width_);

  //  Signal that row count has changed
  emit dimensionsChanged();
  endResetModel();
}

QHash<int, QByteArray> MemoryByteModel::roleNames() const {
  using M = MemoryRoles::Roles;
  // Use a type alias so that auto-formatting is less ugly.
  using T = QHash<int, QByteArray>;
  static T ret = {{Qt::DisplayRole, "display"},
                  {Qt::ToolTipRole, "toolTip"},
                  {Qt::TextAlignmentRole, "textAlign"},
                  {M::Type, "type"},
                  {M::Highlight, "highlight"}};
  return ret;
}

int MemoryByteModel::rowCount(const QModelIndex &) const { return height(); }

int MemoryByteModel::columnCount(const QModelIndex &) const {
  //  Number of binary numbers in row plus row number and ascii representation
  Q_ASSERT(column_->Total() == (width_ + 4));
  return column_->Total();
}

int MemoryByteModel::bytesPerRow() const { return column_->bytesPerLine(); }

QVariant MemoryByteModel::data(const QModelIndex &index, int role) const {
  static pepp::settings::AppSettings settings;
  if (!index.isValid()) return QVariant();

  // The index returns the requested row and column information
  // The first column is the line number, which we ignore
  const int row = index.row();
  const int col = index.column();
  const int i = memoryOffset(index);

  using M = MemoryRoles::Roles;
  switch (role) {
  //  Determines which delegate to assign in grid
  case M::Type:
    //  First column is formatted row number
    if (col == column_->LineNo()) return QString("lineNo");
    //  Last column is ascii representation of data
    if (col == column_->Ascii()) return QVariant("ascii");
    if (col == column_->Border1() || col == column_->Border2()) return QVariant("border");

    return QVariant("cell");
  case Qt::DisplayRole:
    //  First column is formatted row number
    if (col == column_->LineNo()) return QStringLiteral("%1").arg(row * width_, 4, 16, QLatin1Char('0')).toUpper();
    //  Last column is ascii representation of data
    else if (col == column_->Ascii()) return ascii(row);
    else if (col == column_->Border1() || col == column_->Border2()) return {};
    else if (i < 0) return QVariant("");

    //  Show data in hex format
    return QStringLiteral("%1").arg(memory_->read(i), 2, 16, QLatin1Char('0')).toUpper();
  case M::Highlight:
    // Don't higlight non-memory-valued cells.
    if (col == column_->LineNo() || col == column_->Ascii() || col == column_->Border1() || col == column_->Border2())
      return (int)MemoryHighlight::None;
    return (int)memory_->status(i);

  case Qt::TextAlignmentRole:
    if (col == column_->Ascii()) return QVariant(Qt::AlignLeft);
    //  Default for all other cells
    return QVariant(Qt::AlignHCenter);
  case Qt::ToolTipRole:
    //  Handle invalid index
    if (i < 0) return {};
    //   Only show for memory
    if (col >= column_->CellStart() && col <= column_->CellEnd()) {
      static const auto convert = [](const OpcodeModel *opcodes, const quint8 v, bool prev = false) {
        if (!opcodes) return QStringLiteral("");
        else if (auto index = opcodes->indexFromOpcode(v); index == -1) return QStringLiteral("");
        else if (const auto mnemonic = opcodes->data(opcodes->index(index)); prev)
          return QStringLiteral("<br>Previous Opcode: %1").arg(mnemonic.toString());
        else return QStringLiteral("<br>Opcode: %1").arg(mnemonic.toString());
      };
      //  toUpper will work on entire string literal. Separate hex
      //  values and cast to upper case as separate strings
      const auto mem = QStringLiteral("%1").arg(i, 4, 16, QLatin1Char('0')).toUpper();
      auto current = memory_->read(i);
      const auto newH = QStringLiteral("%1").arg(current, 2, 16, QLatin1Char('0')).toUpper();
      const auto newOpcode = convert(mnemonics_, current, false);

      auto old = memory_->readPrevious(i);
      QString trailer;
      if (old) {
        const auto oldH = QStringLiteral("%1").arg(*old, 2, 16, QLatin1Char('0')).toUpper();
        const auto oldOpcode = convert(mnemonics_, *old, true);
        trailer = QStringLiteral("<br>Previous Hex: 0x%1<br>"
                                 "Previous Unsigned Decimal: %2<br>"
                                 "Previous Binary: 0b%3"
                                 "%4")
                      .arg(oldH, QString::number(*old), u"%1"_s.arg(*old, 8, 2, QChar('0')), oldOpcode);
      }

      return QStringLiteral("<b>Memory Location: 0x%1</b><br>"
                            "Hex: 0x%2<br>"
                            "Unsigned Decimal: %3<br>"
                            "Binary: 0b%4"
                            "%5"
                            "%6")
          .arg(mem, newH, QString::number(current), u"%1"_s.arg(current, 8, 2, QChar('0')), newOpcode, trailer)
          .trimmed();
    } else return {};
  }

  return QVariant();
}

bool MemoryByteModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (role != Qt::DisplayRole) return false;
  const int i = memoryOffset(index), hex = (int)std::strtol(value.toString().toStdString().c_str(), NULL, 16);
  if (i < 0) return false;
  else if (const auto current = data(index, role); current != hex) {
    memory_->write(i, hex);
    emit dataChanged(index, index);
    return true;
  } else return false;
}

Qt::ItemFlags MemoryByteModel::flags(const QModelIndex &index) const {
  if (!index.isValid() || index.column() < column_->CellStart() || index.column() > column_->CellEnd())
    return Qt::NoItemFlags;

  return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void MemoryByteModel::clear() {
  memory_->clear();
  emit dataChanged(index(0, 0), index(height() - 1, column_->Ascii()));
}

//  Convert from cellIndex to memory location
//  Model is calculated as (col, row)
std::size_t MemoryByteModel::memoryOffset(const QModelIndex &index) const {
  const std::size_t offset = index.row() * width_ + (index.column() - column_->CellStart());
  const auto size = memory_->byteCount();
  if (offset >= size) return -1;

  //  Test if index is inside data model
  if (index.row() < 0 && std::cmp_greater_equal(index.row(), height())) return -1;

  //  First column is line number. Skip
  if (index.column() <= 0 && std::cmp_greater_equal(index.column(), width_)) return -1;

  //  First column is line number. Return cell index to first edit column
  return offset;
}

//  Convert to location in model from memory location
QModelIndex MemoryByteModel::memoryIndex(std::size_t index) {
  //  Check for memory overflow
  const auto size = memory_->byteCount();
  if (index >= size) return QModelIndex();

  const int row = std::floor(index / width_);
  const int col = index % width_;

  //  Test if index is inside data model
  if (row < 0 && std::cmp_greater_equal(row, height())) return QModelIndex();

  //  First column is line number. Skip
  if (col <= 0 && std::cmp_greater_equal(col, width_)) return QModelIndex();

  //  First column is line number. Ignore
  return QAbstractItemModel::createIndex(row, col + column_->CellStart());
}

QString MemoryByteModel::ascii(const int row) const {
  const auto size = memory_->byteCount();
  const int start = row * width_;
  const int end = start + width_;
  const auto len = end - start;
  QString edit(static_cast<qsizetype>(len), ' ');
  for (int i = 0; i < len; ++i) {
    if (std::cmp_greater_equal(i + start, size)) continue;
    QChar c(memory_->read(start + i));
    if (c.isPrint()) edit[i] = c;
    else edit[i] = '.';
  }
  return edit;
}

void MemoryByteModel::onDataChanged(quint32 start, quint32 end) {
  // Conservatively repaint the entire
  auto startIndex = memoryIndex(start).siblingAtColumn(1);
  auto endIndex = memoryIndex(end).siblingAtColumn(columnCount() - 1);
  static const auto roles = QList<int>{Qt::DisplayRole, (int)MemoryRoles::Highlight};
  if (!(startIndex.isValid() && endIndex.isValid())) return;

  if (!(startIndex.isValid() && endIndex.isValid())) {
    static const char *const e = "Bad column access";
    qCritical(e);
    throw std::logic_error(e);
  }
  emit dataChanged(startIndex, endIndex, roles);
}
