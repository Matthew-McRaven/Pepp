#include "changelogmodel.hpp"

#include <QFileInfo>
#include <QQmlEngine>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

Change::Change(QString body, int priority, int ref, QObject *parent)
    : QObject(parent), _body(body), _priority(priority), _ghRef(ref) {}

Section::Section(QString title, QObject *parent) : QObject(parent), _title(title) {}

void Section::add_change(Change *change) {
  change->setParent(this);
  _changes.append(change);
}

Version::Version(QVersionNumber ver, QDate date, QString blurb, QObject *parent)
    : QObject(parent), _version(ver), _date(date), _blurb(blurb) {}

void Version::add_section(Section *section) {
  section->setParent(this);
  _sections.push_back(section);
}

static const char *path = ":/changelog/changelog.db";
ChangelogModel::ChangelogModel() : _versions() {
  if (auto it = QFileInfo(path); it.exists()) loadFromDB();
}

void ChangelogModel::loadFromDB() {
  // Nested scope so db will be destroyed before QSqlDatabase::removeDatabase.
  QString cname;
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    // VFS to allow using QFile (and QRC files). RO needed or will crash.
    db.setConnectOptions("QSQLITE_USE_QT_VFS;QSQLITE_OPEN_READONLY");
    db.setDatabaseName(path);
    if (!db.open()) {
      qFatal("%s", db.lastError().text().toStdString().c_str());
    }
    // Create Version objects for each row in version table, include special handling for NULL/0 cases
    QMap<int, Version *> versions;
    {
      QSqlQuery q("SELECT id,version,date,blurb FROM versions", db);
      q.exec();
      while (q.next()) {
        auto date = QDate::fromString(q.value(2).toString(), "yyyy-MM-dd");
        auto tmp = new Version(QVersionNumber::fromString(q.value(1).toString()), date, q.value(3).toString(), this);
        QQmlEngine::setObjectOwnership(tmp, QQmlEngine::CppOwnership);
        if (auto it = versions.find(q.value(0).toInt()); it != versions.end()) qFatal("Duplicate version");
        versions[q.value(0).toInt()] = tmp;
        _versions.push_back(tmp);
      }
    }
    // Sort _versions by SemVer
    std::sort(_versions.begin(), _versions.end(), [](Version *a, Version *b) { return a->version() > b->version(); });

    // Map type string values to integers for ease of sorting.
    {
      QSqlQuery q("SELECT rowid, name FROM types", db);
      q.exec();
      while (q.next()) {
        _types[q.value(0).toInt()] = q.value(1).toString();
        _types_rev[q.value(1).toString()] = q.value(0).toInt();
      }
    }

    // Construct appropriate sections and changes as we iteratre over changes table.
    {
      QSqlQuery q("SELECT version, type, priority, ref, message FROM changes", db);
      q.exec();
      // Map (version, type) to section.
      QMap<std::tuple<int, int>, Section *> sections;
      while (q.next()) {
        // Unpack SQL values based on query order.
        auto version = versions[q.value(0).toInt()];
        int type = q.value(1).toInt();
        int priority = q.value(2).toInt();
        QString message = q.value(4).toString();

        // Attempt to find existing section (if it exists), or create one.
        auto section_key = std::make_tuple<int, int>(q.value(0).toInt(), q.value(1).toInt());
        Section *target_section = nullptr;
        if (auto it = sections.find(section_key); it != sections.end()) target_section = *it;
        else {
          target_section = new Section(_types[type], version);
          QQmlEngine::setObjectOwnership(target_section, QQmlEngine::CppOwnership);
          sections[section_key] = target_section;
          version->add_section(target_section);
        }

        // Add the current change to that section
        auto tmp = new Change(message, priority, q.value(3).toInt(), nullptr);
        QQmlEngine::setObjectOwnership(tmp, QQmlEngine::CppOwnership);
        target_section->add_change(tmp);
      }
    }
    // Provide a stable sort order for sections across all versions.
    for (auto &version : _versions) {
      auto sections = version->sections();
      std::sort(sections.begin(), sections.end(),
                [this](Section *a, Section *b) { return _types_rev[a->title()] < _types_rev[b->title()]; });
    }
    db.close();
    cname = db.connectionName();
  }
  // Silence annoying error on re-open changelog.
  // QSqlDatabasePrivate::addDatabase: duplicate connection name 'qt_sql_default_connection', old connection removed.
  QSqlDatabase::removeDatabase(cname);
}

int ChangelogModel::rowCount(const QModelIndex &parent) const { return _versions.size(); }

QVariant ChangelogModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  switch (role) {
  case Qt::DisplayRole: return QVariant::fromValue(_versions[index.row()]);
  case Qt::UserRole + 1: return _versions[index.row()]->version_str();
  default: return QVariant();
  }
}

QHash<int, QByteArray> ChangelogModel::roleNames() const {
  auto ret = QAbstractListModel::roleNames();
  ret[Qt::UserRole + 1] = "version_str";
  return ret;
}

QVersionNumber ChangelogModel::min() const {
  if (_versions.empty()) return {};
  return _versions.last()->version();
}

QVersionNumber ChangelogModel::max() const {
  if (_versions.empty()) return {};
  return _versions.first()->version();
}

ChangelogFilterModel::ChangelogFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void ChangelogFilterModel::setSourceModel(QAbstractItemModel *sourceModel) {
  if (sourceModel == this->sourceModel()) return;
  else if (auto casted = qobject_cast<ChangelogModel *>(sourceModel); casted == nullptr)
    qFatal("ChangelogFilterModel only accepts ChangelogModel as sourceModel");
  QSortFilterProxyModel::setSourceModel(sourceModel);
  if (_min.isNull()) setMin(static_cast<ChangelogModel *>(sourceModel)->min().toString());
  if (_max.isNull()) setMax(static_cast<ChangelogModel *>(sourceModel)->max().toString());
  emit sourceModelChanged();
}

void ChangelogFilterModel::setMin(QString min_str) {
  QVersionNumber min = QVersionNumber::fromString(min_str);
  if (_min == min) return;
  _min = min;
  invalidateRowsFilter();
  // qDebug() << QStringLiteral("Min set: [%1, %2]").arg(_min.toString(), _max.toString());
  emit minChanged();
}

void ChangelogFilterModel::setMax(QString max_str) {
  QVersionNumber max = QVersionNumber::fromString(max_str);
  if (_max == max) return;
  _max = max;
  invalidateRowsFilter();
  // qDebug() << QStringLiteral("Max set: [%1, %2]").arg(_min.toString(), _max.toString());
  emit maxChanged();
}

bool ChangelogFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  auto sm = sourceModel();
  if (!sm) return false;
  auto index = sm->index(source_row, 0, source_parent);
  auto version = sm->data(index, Qt::DisplayRole).value<Version *>()->version();
  // qDebug() << reinterpret_cast<const int *>(this) << _min.toString() << _max.toString() << version.toString();
  if (!_min.isNull() && version < _min) return false;
  if (!_max.isNull() && version > _max) return false;
  return true;
}
