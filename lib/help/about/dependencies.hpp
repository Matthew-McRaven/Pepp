#pragma once
#include <QtCore>
#include <QtQmlIntegration>

namespace about {
struct Dependency {
  QString name, url, licenseName, licenseSPDXID, licenseText;
  bool devDependency;
};
QList<Dependency> dependencies();

class DependencyRoles : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")
public:
  enum RoleNames {
    Name = Qt::UserRole,
    URL = Qt::UserRole + 1,
    LicenseName = Qt::UserRole + 2,
    LicenseSPDXID = Qt::UserRole + 3,
    LicenseText = Qt::UserRole + 4,
    DevDependency = Qt::UserRole + 5
  };
  Q_ENUM(RoleNames)
  static DependencyRoles *instance();
  // Prevent copying and assignment
  DependencyRoles(const DependencyRoles &) = delete;
  DependencyRoles &operator=(const DependencyRoles &) = delete;

private:
  DependencyRoles() : QObject(nullptr) {}
};

class Dependencies : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  explicit Dependencies(QObject *parent = nullptr);
  ~Dependencies() override = default;
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  QList<about::Dependency> _deps;
};

} // namespace about
