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
#pragma once
#include <QAbstractListModel>
#include <QtQmlIntegration>
#include <deque>
#include "project/pep10.hpp"

// Factory to ensure class invariants of project are maintained.
// Must be a singleton to call methods on it.
// Can't seem to call Q_INVOKABLE on an uncreatable type.
class ProjectModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ _rowCount NOTIFY rowCountChanged)
  QML_ELEMENT

public:
  enum class Roles {
    ProjectPtrRole = Qt::UserRole + 1,
    NameRole,
    DescriptionRole,
    DirtyRole,
    PathRole,
  };
  Q_ENUM(Roles);
  Q_INVOKABLE int roleForName(const QString &name) const;
  explicit ProjectModel(QObject *parent = nullptr) : QAbstractListModel(parent) {};
  // Helper to expose rowCount as a property to QML.
  int _rowCount() const { return rowCount({}); }
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  Q_INVOKABLE Pep_MA *pep9MA2(pepp::Features features);
  Q_INVOKABLE Pep_MA *pep10MA2(pepp::Features features);
  Q_INVOKABLE Pep_ISA *pep10ISA();
  Q_INVOKABLE Pep_ISA *pep9ISA();
  Q_INVOKABLE Pep_ASMB *pep10ASMB(pepp::Abstraction abstraction);
  Q_INVOKABLE Pep_ASMB *pep9ASMB();
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                int destinationChild) override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE QString describe(int index) const;
  Q_INVOKABLE int rowOf(const QObject *item) const;
public slots:
  Q_INVOKABLE bool onSave(int index);
  Q_INVOKABLE bool onSaveAs(int index, const QString &extension);
signals:
  void rowCountChanged(int);

private:
  struct Data {
    std::unique_ptr<QObject> impl = nullptr;
    QString name = "";
    bool isDirty = false;
    QString path = "";
  };
  std::deque<Data> _projects = {};
  // Emplace obj at end of _projects and register obj->markDirty to setData dirty;
  void appendProject(std::unique_ptr<QObject> &&obj);
};

enum class CompletionState {
  INCOMPLETE,
  PARTIAL,
  COMPLETE,
};

struct ProjectType {
  QString name{};
  QString levelText{};
  QString details{};
  QString chapter{};
  QString description{};
  pepp::Architecture arch = pepp::Architecture::NO_ARCH;
  pepp::Abstraction level = pepp::Abstraction::NO_ABS;
  pepp::Features features = pepp::Features::None;
  CompletionState state = CompletionState::INCOMPLETE;
  int edition = 0;
  bool placeholder = false;
  // Used to hide items that only differ by features.
  bool is_duplicate_feature = false;
};

class ProjectTypeModel : public QAbstractTableModel {
  Q_OBJECT
  QML_ELEMENT

public:
  enum class Roles {
    NameRole = Qt::UserRole + 1,
    DescriptionRole,
    ArchitectureRole,
    LevelRole,
    CombinedArchLevelRole,
    CompleteRole,
    PartiallyCompleteRole,
    ColumnTypeRole,
    EditionRole,
    PlaceholderRole,
    LevelTextRole,
    DetailsRole,
    ChapterRole,
    FeatureRole,
    IsDuplicateFeature,
  };
  Q_ENUM(Roles);
  explicit ProjectTypeModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  QList<ProjectType> _projects{};
};

class ProjectTypeFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  // Filter may either be architecture OR edition. Setting one clears the other.
  Q_PROPERTY(pepp::Architecture architecture READ architecture WRITE setArchitecture NOTIFY architectureChanged)
  Q_PROPERTY(int edition READ edition WRITE setEdition NOTIFY editionChanged)
  Q_PROPERTY(bool showIncomplete READ showIncomplete WRITE setShowIncomplete NOTIFY showIncompleteChanged)
  Q_PROPERTY(bool showPartiallyComplete READ showPartial WRITE setShowPartial NOTIFY showPartialChanged)
  Q_PROPERTY(bool showDuplicateFeatures READ showDuplicateFeatures WRITE setShowDuplicateFeatures NOTIFY
                 showDuplicateFeaturesChanged)
  QML_ELEMENT

public:
  explicit ProjectTypeFilterModel(QObject *parent = nullptr);
  pepp::Architecture architecture() const { return _architecture; }
  int edition() const { return _edition; }
  void setArchitecture(pepp::Architecture arch);
  void setEdition(int edition);
  bool showIncomplete() const { return _showIncomplete; }
  void setShowIncomplete(bool value);
  bool showPartial() const { return _showPartial; }
  void setShowPartial(bool value);
  bool showDuplicateFeatures() const { return _showDuplicateFeatures; }
  void setShowDuplicateFeatures(bool value);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
signals:
  void architectureChanged();
  void editionChanged();
  void showIncompleteChanged();
  void showPartialChanged();
  void showDuplicateFeaturesChanged();

private:
  pepp::Architecture _architecture = pepp::Architecture::NO_ARCH;
  int _edition = 0;
  bool _showIncomplete = false, _showPartial = false, _showDuplicateFeatures = true;
};
