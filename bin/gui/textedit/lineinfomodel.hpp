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
#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQuickTextDocument>
#include <QTextBlockUserData>

// Store breakpoint info in block user data.
// As lines are inserted/deleted/modified in the UI,
// the text edit will move the blocks around while keeping the user data current.
// This is a workaround for the lack of linesInserted/linesRemoved signals in QTextDocument.
struct LineInfoData : public QTextBlockUserData {
  virtual ~LineInfoData() = default;
  bool allowsBP = false, hasBP = false, allowsNumber = true;
  int number = -1;
};

#define SHARED_CONSTANT(type, name, value)                                                                             \
  static inline const type name = value;                                                                               \
  Q_PROPERTY(type name MEMBER name CONSTANT)

class LineInfoConstants : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  // boolean indicating if a line is allowed to have a breakpoint.
  SHARED_CONSTANT(quint32, ALLOWS_BP, Qt::UserRole + 1);
  // boolean indicating if the line currently has a breakpoint.
  SHARED_CONSTANT(quint32, HAS_BP, Qt::UserRole + 2);
  // boolean indicating if current line contribute to line numbering.
  SHARED_CONSTANT(quint32, HAS_NUMBER, Qt::UserRole + 3);
  // unsigned int with the logical line number of the current line.
  SHARED_CONSTANT(quint32, NUMBER, Qt::UserRole + 4);
};

class LineInfoModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QQuickTextDocument *document WRITE setDocument)
public:
  explicit LineInfoModel(QObject *parent = nullptr);
  ~LineInfoModel() override = default;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;
public slots:
  void setDocument(QQuickTextDocument *doc);
  // Allow QML to toggle breakpoint state.
  Q_INVOKABLE void toggleBreakpoint(int line);
  // To capture events about the document changing
  void onContentsChange(int position, int charsRemoved, int charsAdded);

private:
  void reset();
  bool updateAllowsBreakpoint(int start, int end);
  bool updateLineNumbers(int from);
  LineInfoData *userDataForBlock(QTextBlock &block) const;
  QTextDocument *_doc = nullptr;
  int _linecount = 0;
};
Q_DECLARE_METATYPE(LineInfoModel);
