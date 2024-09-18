#pragma once
#include <QAbstractListModel>
#include <QMap>
#include <QObject>
#include <QVersionNumber>

class Change : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString body READ body CONSTANT)
  Q_PROPERTY(int priority READ priority CONSTANT)
public:
  Change(QString body, int priority, QObject *parent = nullptr);
  QString body() const { return _body; }
  int priority() const { return _priority; }

private:
  QString _body;
  int _priority, _type;
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
public:
  Version(QVersionNumber ver, QObject *parent = nullptr);
  void add_section(Section *section);
  QVersionNumber version() const { return _version; }
  Q_INVOKABLE QString version_str() const { return _version.toString(); }
  Q_INVOKABLE int major() const { return _version.majorVersion(); }
  Q_INVOKABLE int minor() const { return _version.minorVersion(); }
  Q_INVOKABLE int micro() const { return _version.microVersion(); }
  Q_INVOKABLE QList<Section *> sections() const { return _sections; }

private:
  QVersionNumber _version;
  QList<Section *> _sections;
};

class ChangelogModel : public QAbstractListModel {
  Q_OBJECT
public:
  ChangelogModel();
  // QAbstractItemModel interface
  int rowCount(const QModelIndex &parent) const override;
  // Returns lots of Version objects
  QVariant data(const QModelIndex &index, int role) const override;

private:
  QList<Version *> _versions{};
  QMap<int, QString> _types{};
  QMap<QString, int> _types_rev{};
  void loadFromDB();
};

class ChangelogFilterModel {};
