#pragma once
#include <QtCore/QtCore>
#include "help_globals.hpp"

namespace help {
struct HELP_EXPORT Entry {
  enum class Category {
    Root,
    About,
    Figure,
    ISAGreenCard,
    Text,
  } category;
  Q_ENUM(Category)
  // TBD on how to filter these items.
  int tags;
  // Display name in help system; path to QML file which can display it.
  QString name, delgate;
  // Props which will be injected into the delegate.
  QVariantMap props = {};
  // Instead of showing as items under the current element, append visible children in the current view.
  // From the model's perspective, report this item as having no children.
  bool showInParent = false;
  // Items which occur under this item.
  QList<QSharedPointer<Entry>> children = {};
};

} // namespace help
