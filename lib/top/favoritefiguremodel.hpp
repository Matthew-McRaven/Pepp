#pragma once
#include <QtCore/qabstractitemmodel.h>
#include <qqmlintegration.h>
namespace builtins {
class Registry;
class Figure;
} // namespace builtins

class FavoriteFigureModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ _rowCount NOTIFY rowCountChanged)
  QML_ELEMENT
public:
  enum class Roles {
    FigurePtrRole = Qt::UserRole + 1,
    NameRole,
    TypeRole,
    DescriptionRole,
  };
  Q_ENUM(Roles);
  explicit FavoriteFigureModel(QObject *parent = nullptr);
  // Helper to expose rowCount as a property to QML.
  int _rowCount() const { return rowCount({}); }
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
signals:
  void rowCountChanged(int);

private:
  QSharedPointer<builtins::Registry> _registry;
  QList<QSharedPointer<const builtins::Figure>> _figures;
};
