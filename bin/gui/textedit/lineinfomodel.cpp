/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include "lineinfomodel.hpp"
#include <QQuickTextDocument>
#include <QRegularExpression>
#include "qtextobject.h"

LineInfoModel::LineInfoModel(QObject *parent) : QAbstractListModel(parent) {}

int LineInfoModel::rowCount(const QModelIndex &parent) const { return _linecount; }

QVariant LineInfoModel::data(const QModelIndex &index, int role) const {
  // _doc is a nullptr when class is init'ed, so we must guard against null deref.
  if (!_doc)
    return QVariant();
  auto block = _doc->findBlockByNumber(index.row());
  auto data = userDataForBlock(block);

  switch (role) {
  case LineInfoConstants::ALLOWS_BP:
    return QVariant(data->allowsBP);
  case LineInfoConstants::HAS_BP:
    return QVariant(data->hasBP);
  case LineInfoConstants::HAS_NUMBER:
    return QVariant(data->allowsNumber);
  case LineInfoConstants::NUMBER:
    return QVariant(data->number);
  default:
    return QVariant();
  }
}

bool LineInfoModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  // _doc is a nullptr when class is init'ed, so we must guard against null deref.
  if (!_doc)
    return false;

  auto block = _doc->findBlockByNumber(index.row());
  auto data = userDataForBlock(block);

  // Reject updates that don't change the value. This saves a UI refresh.
  switch (role) {
  case LineInfoConstants::ALLOWS_BP:
    if (data->allowsBP == value.toBool())
      return false;
    data->allowsBP = value.toBool();
    break;
  case LineInfoConstants::HAS_BP:
    if (data->hasBP == value.toBool())
      return false;
    data->hasBP = value.toBool();
    break;
  default:
    return false;
  }

  emit dataChanged(index, index, {role});
  return true;
}

Qt::ItemFlags LineInfoModel::flags(const QModelIndex &index) const { return QAbstractListModel::flags(index); }

QHash<int, QByteArray> LineInfoModel::roleNames() const {
  // Extend QStandardItemModel role names
  QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
  roles[LineInfoConstants::ALLOWS_BP] = "allowsBP";
  roles[LineInfoConstants::HAS_BP] = "hasBP";
  roles[LineInfoConstants::HAS_NUMBER] = "hasNumber";
  roles[LineInfoConstants::NUMBER] = "number";
  return roles;
}

void LineInfoModel::setDocument(QQuickTextDocument *doc) {
  // If we are passed an empty document, don't attempt to dereference it.
  if (doc == nullptr) {
    _doc = nullptr;
    reset();
    return;
  }
  // Don't trigger an update if the new document is the old document.
  else if (auto textDocument = doc->textDocument(); _doc == textDocument)
    return;
  else {
    // Disconnect old document to avoid memory leaks.
    if (_doc)
      disconnect(_doc, &QTextDocument::contentsChange, this, &LineInfoModel::onContentsChange);
    _doc = textDocument;
    reset();
    connect(textDocument, &QTextDocument::contentsChange, this, &LineInfoModel::onContentsChange);
  }
}

void LineInfoModel::onContentsChange(int position, int charsRemoved, int charsAdded) {
  // _doc is a nullptr when class is init'ed, so we must guard against null deref.
  if (_doc == nullptr)
    return;

  // I would prefer to avoid beginResetModel here, since it would completely re-render the UI.
  // Instead, I will track how many lines were added / removed and emit the corresponding events.
  // This will allow the UI to only re-render the affected lines.
  enum { MODIFY = 0, REMOVE = 1, ADD = 2 } change = MODIFY;
  auto newcount = _doc->blockCount();
  auto blockdelta = newcount - _linecount;

  // Block from which rows were inserted or deleted.
  auto fromBlock = _doc->findBlock(position).blockNumber();
  // Determine which event to emit, and notify about an upcoming modification.
  if (blockdelta < 0) {
    change = REMOVE;
    emit beginRemoveRows(QModelIndex(), fromBlock, fromBlock - blockdelta - 1);
  } else if (blockdelta > 0) {
    change = ADD;
    // TODO: Maybe need to invert the order of the arguments here.
    emit beginInsertRows(QModelIndex(), fromBlock, fromBlock + blockdelta - 1);
  }
  // Now that we no longer need old line count, we can safely update it.
  _linecount = newcount;

  auto startBlock = _doc->findBlock(position);
  // findBlock(position+charsAdded) can go past the end of the document.
  // Instead, we find either: the last valid block, of the block at position+charsAdded.
  auto endBlock = startBlock;
  for (auto it = startBlock; it.isValid() && it.position() <= position + charsAdded; it = it.next())
    endBlock = it;

  auto previous = startBlock.previous();
  // An insert may modify the previous block. At the begining of the document, the previous block is not valid.
  startBlock = previous.isValid() ? previous : startBlock;
  const auto startLine = startBlock.blockNumber(), endLine = endBlock.blockNumber();

  // Track if any data values were updated, so that we can make the UI re-paint those rows.
  bool changedBP = updateAllowsBreakpoint(startLine, endLine);
  bool changedLineNum = updateLineNumbers(startLine);

  // Complete UI update.
  if (change == REMOVE)
    emit endRemoveRows();
  else if (change == ADD)
    emit endInsertRows();

  // Re-render the UI if any row's data changed. Always trigger on insert/delete,
  // because I am not sure if the UI will pick up the changes outside of the inserted regions.
  // Trigger after end.*Rows() because of API requirements.
  if (changedLineNum)
    emit dataChanged(index(startLine, 0), index(_linecount - 1, 0),
                     {LineInfoConstants::ALLOWS_BP, LineInfoConstants::HAS_BP, LineInfoConstants::HAS_NUMBER,
                      LineInfoConstants::NUMBER});
  else if (changedBP || (change != MODIFY))
    emit dataChanged(index(startLine, 0), index(endLine, 0),
                     {LineInfoConstants::ALLOWS_BP, LineInfoConstants::HAS_BP, LineInfoConstants::HAS_NUMBER,
                      LineInfoConstants::NUMBER});
}

void LineInfoModel::toggleBreakpoint(int line) {
  auto index = this->index(line, 0);
  if (!data(index, LineInfoConstants::ALLOWS_BP).toBool())
    return;
  auto current_state = data(index, LineInfoConstants::HAS_BP).toBool();
  setData(index, !current_state, LineInfoConstants::HAS_BP);
}

void LineInfoModel::reset() {
  emit beginResetModel();

  // _doc is a nullptr when class is init'ed, so we must guard against null deref.
  if (_doc) {
    _linecount = _doc->blockCount();
    updateAllowsBreakpoint(0, _linecount);
    updateLineNumbers(0);
  } else
    _linecount = 0;

  emit endResetModel();
}

bool LineInfoModel::updateAllowsBreakpoint(int start, int end) {
  // TODO: This misses dot commands, which should not have breakpoints.
  // However, I cannot do better without a lexer, which I will add in the future.
  static const auto notExecutable = QRegularExpression(R"(^\W*(;|$))");
  static const auto isWhitespace = QRegularExpression(R"(^\W*$)");

  // _doc is a nullptr when class is init'ed, so we must guard against null deref.
  if (_doc == nullptr)
    return false;

  // Track if any value in the range was updated.
  // If a line became non-exectuable due to an added newline, attempt to move the BP to the next line.
  bool changed = false, deferBP = false;
  for (auto it = _doc->findBlockByLineNumber(start); it.isValid() && it.blockNumber() <= end; it = it.next()) {
    auto data = userDataForBlock(it);
    auto text = it.text();
    bool allows_bp = !notExecutable.match(text).hasMatch();
    changed |= (data->allowsBP != allows_bp);
    data->allowsBP = allows_bp;

    // Strip breakpoints from lines that are no longer breakpointable.
    if (!allows_bp && data->hasBP) {
      // If the line became blank, then the next line is probably executable.
      // So, defer the breakpoint to the next line.
      deferBP = isWhitespace.match(text).hasMatch();
      changed |= true;
      data->hasBP = false;
    }
    // Allow deferred breakpoints to move across multiple blank lines.
    else if (!allows_bp && deferBP)
      deferBP = isWhitespace.match(text).hasMatch();
    // Attempt to consume deferred BP.
    else if (allows_bp && deferBP) {
      changed |= (data->hasBP != true);
      data->hasBP = true;
      deferBP = false;
    }
    // Prevent a BP from being deferred across more than 1 one line.
    else {
      deferBP = false;
    }
  }
  return changed;
}

bool LineInfoModel::updateLineNumbers(int from) {
  bool changed = false;
  for (auto it = _doc->findBlockByLineNumber(from); it.isValid(); it = it.next()) {
    auto data = userDataForBlock(it);
    // Convert from 0-indexed to 1-indexed.
    auto number = it.blockNumber() + 1;
    // changed |= (data->allowsNumber != allowsNumber);
    changed |= (data->number != number);
    // data->allowsNumber = allowsNumber;
    data->number = number;
  }

  return changed;
}

LineInfoData *LineInfoModel::userDataForBlock(QTextBlock &block) const {
  if (auto data = block.userData(); data != nullptr) {
    if (auto typed_data = dynamic_cast<LineInfoData *>(data); typed_data != nullptr)
      return typed_data;
    else
      throw std::logic_error("Block has user data, but it is not of the expected type.");
  }

  // Ownership transfers to block.
  // See: https://doc.qt.io/qt-6/qtextblock.html#setUserData
  auto data = new LineInfoData();
  block.setUserData(data);
  return data;
}
