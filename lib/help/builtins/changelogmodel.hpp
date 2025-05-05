#pragma once
#include <QAbstractListModel>
#include <QDate>
#include <QMap>
#include <QObject>
#include <QSortFilterProxyModel>
#include <QVersionNumber>
#include <QtQmlIntegration>

class Change : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString body READ body CONSTANT)
  Q_PROPERTY(int priority READ priority CONSTANT)
  Q_PROPERTY(int ghRef READ ghRef CONSTANT)

public:
  Change(QString body, int priority, int ghRef = 0, QObject *parent = nullptr);
  QString body() const { return _body; }
  int priority() const { return _priority; }
  int ghRef() const { return _ghRef; }

private:
  QString _body;
  int _priority, _ghRef;
};

class Section : public QObject {
  Q_OBJECT
  Q_PROPERTY(QList<Change *> changes READ changes CONSTANT)
  Q_PROPERTY(QString title READ title CONSTANT)

public:
  // Section(QString title);
  Section(QString title, QObject *parent = nullptr);
  void add_change(Change *change);
  Q_INVOKABLE QList<Change *> changes() const { return _changes; }
  Q_INVOKABLE QString title() const { return _title; }

private:
  QList<Change *> _changes;
  QString _title;
};

class Version : public QObject {
  Q_OBJECT
  Q_PROPERTY(QList<Section *> sections READ sections CONSTANT)
  Q_PROPERTY(QString version READ version_str CONSTANT)
  Q_PROPERTY(int major READ major CONSTANT)
  Q_PROPERTY(int minor READ minor CONSTANT)
  Q_PROPERTY(int micro READ micro CONSTANT)
  Q_PROPERTY(QString blurb READ blurb CONSTANT)
  Q_PROPERTY(bool hasDate READ hasDate CONSTANT)
  Q_PROPERTY(QDate date READ date CONSTANT)

public:
  Version(QVersionNumber ver, QDate date, QString blurb = "", QObject *parent = nullptr);
  void add_section(Section *section);
  QVersionNumber version() const { return _version; }
  Q_INVOKABLE QString version_str() const { return _version.toString(); }
  Q_INVOKABLE int major() const { return _version.majorVersion(); }
  Q_INVOKABLE int minor() const { return _version.minorVersion(); }
  Q_INVOKABLE int micro() const { return _version.microVersion(); }
  Q_INVOKABLE bool hasDate() const { return _date.isValid(); }
  Q_INVOKABLE QDate date() const { return _date; }
  Q_INVOKABLE QString blurb() const { return _blurb; }
  Q_INVOKABLE QList<Section *> sections() const { return _sections; }

private:
  QVersionNumber _version;
  QDate _date;
  QString _blurb;
  QList<Section *> _sections;
};

class ChangelogModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

public:
  ChangelogModel();
  // QAbstractItemModel interface
  int rowCount(const QModelIndex &parent) const override;
  // Returns lots of Version objects
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVersionNumber min() const;
  QVersionNumber max() const;

private:
  QList<Version *> _versions{};
  QMap<int, QString> _types{};
  QMap<QString, int> _types_rev{};
  void loadFromDB();
};

class ChangelogFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(QAbstractItemModel *model READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
  Q_PROPERTY(QString min READ min WRITE setMin NOTIFY minChanged)
  Q_PROPERTY(QString max READ max WRITE setMax NOTIFY maxChanged)
  QML_ELEMENT

public:
  explicit ChangelogFilterModel(QObject *parent = nullptr);
  void setSourceModel(QAbstractItemModel *sourceModel) override;
  QString min() const { return _min.toString(); }
  void setMin(QString min);
  QString max() const { return _max.toString(); }
  void setMax(QString max);

signals:
  void sourceModelChanged();
  void minChanged();
  void maxChanged();

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
  QVersionNumber _min = {}, _max = {};
};
