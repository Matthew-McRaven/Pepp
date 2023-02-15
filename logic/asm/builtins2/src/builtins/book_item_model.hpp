#pragma once
#include "QtQmlIntegration/qqmlintegration.h"
#include <QHash>
#include <QStandardItemModel>
#include <QString>
namespace builtins {

#define SHARED_CONSTANT(type, name, value)                                     \
  static inline const type name = value;                                       \
  Q_PROPERTY(type name MEMBER name CONSTANT)

class FigureConstants : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  SHARED_CONSTANT(quint32, FIG_ROLE_KIND, Qt::UserRole + 1);
  SHARED_CONSTANT(quint32, FIG_ROLE_PAYLOAD, Qt::UserRole + 2);
};

class Registry;
class BookModel : public QStandardItemModel {
  Q_OBJECT
public:
  BookModel(QSharedPointer<builtins::Registry> registry);
  QHash<int, QByteArray> roleNames() const override;

private:
  QSharedPointer<builtins::Registry> _registry;
};
} // namespace builtins
Q_DECLARE_METATYPE(builtins::BookModel);
