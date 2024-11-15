#pragma once
#include <QObject>
#include <QtQmlIntegration>
#include "./constants.hpp"

namespace pepp::settings {
class Palette;
class PaletteManager : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int display MEMBER _display CONSTANT);
  Q_PROPERTY(Role path MEMBER _path CONSTANT);
  Q_PROPERTY(Role isSystem MEMBER _isSystem CONSTANT);
  QML_ELEMENT

public:
  enum class Role : int {
    PathRole = Qt::UserRole + 1,
    IsSystemRole,
  };
  explicit PaletteManager(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE void reload();

private:
  struct Entry {
    QString name{}, path{};
    bool isSystem = false;
  };
  QVector<Entry> _palettes{};
  void loadFrom(QString directory);
  static const int _display = Qt::DisplayRole;
  static const Role _path = Role::PathRole;
  static const Role _isSystem = Role::IsSystemRole;
};

} // namespace pepp::settings
