#pragma once
#include "QtQmlIntegration/qqmlintegration.h"
#include <QHash>
#include <QStandardItemModel>
#include <QString>
namespace builtins {

#define SHARED_CONSTANT(type, name, value)                                     \
  static inline const type name = value;                                       \
  Q_PROPERTY(type name MEMBER name CONSTANT)

/*!
 * \brief Contains constants for item model roles to be shared between QML and
 * C++
 *
 * These roles are used to
 */
class FigureConstants : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  //! A role which contains a string description of the scope of the current
  //! item. For example, "book", or "figure". It is used to interpret the
  //! payload field
  SHARED_CONSTANT(quint32, FIG_ROLE_KIND, Qt::UserRole + 1);
  /*!
   * \brief A role which contains figures or help documentation depending on the
   * value of \sa FIG_ROLE_KIND.
   *
   * If FIG_ROLE_KIND == "figure", then this field contains a \sa
   * builtins::Figure If FIG_ROLE_KIND == "book", then this field contains a
   * QVariantList of \sa builtins::Figure.
   */
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
