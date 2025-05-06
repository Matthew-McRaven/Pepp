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
  explicit ProjectModel(QObject *parent = nullptr) : QAbstractListModel(parent) {};
  // Helper to expose rowCount as a property to QML.
  int _rowCount() const { return rowCount({}); }
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  Q_INVOKABLE Pep_ISA *pep10ISA(QVariant delegate);
  Q_INVOKABLE Pep_ISA *pep9ISA(QVariant delegate);
  Q_INVOKABLE Pep_ASMB *pep10ASMB(QVariant delegate, pepp::Abstraction abstraction);
  Q_INVOKABLE Pep_ASMB *pep9ASMB(QVariant delegate);
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent,
                int destinationChild) override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE QString describe(int index) const;
  Q_INVOKABLE int rowOf(const QObject *item) const;
public slots:
  Q_INVOKABLE void onSave(int index);
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
};

enum class CompletionState {
  INCOMPLETE,
  PARTIAL,
  COMPLETE,
};

struct ProjectType {
  QString name{}, description{}, imagePath{};
  pepp::Architecture arch = pepp::Architecture::NO_ARCH;
  pepp::Abstraction level = pepp::Abstraction::NO_ABS;
  CompletionState state = CompletionState::INCOMPLETE;
};

class ProjectTypeModel : public QAbstractTableModel {
  Q_OBJECT
  QML_ELEMENT

public:
  enum class Roles {
    NameRole = Qt::UserRole + 1,
    DescriptionRole,
    ImagePathRole,
    ArchitectureRole,
    LevelRole,
    CompleteRole,
    PartiallyCompleteRole,
    ColumnTypeRole,
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
  Q_PROPERTY(pepp::Architecture architecture READ architecture WRITE setArchitecture NOTIFY architectureChanged)
  Q_PROPERTY(bool showIncomplete READ showIncomplete WRITE setShowIncomplete NOTIFY showIncompleteChanged)
  Q_PROPERTY(bool showPartiallyComplete READ showPartial WRITE setShowPartial NOTIFY showPartialChanged)
  QML_ELEMENT

public:
  explicit ProjectTypeFilterModel(QObject *parent = nullptr);
  pepp::Architecture architecture() const { return _architecture; }
  void setArchitecture(pepp::Architecture arch);
  bool showIncomplete() const { return _showIncomplete; }
  void setShowIncomplete(bool value);
  bool showPartial() const { return _showPartial; }
  void setShowPartial(bool value);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
signals:
  void architectureChanged();
  void showIncompleteChanged();
  void showPartialChanged();

private:
  pepp::Architecture _architecture = pepp::Architecture::NO_ARCH;
  bool _showIncomplete = false, _showPartial = false;
};
