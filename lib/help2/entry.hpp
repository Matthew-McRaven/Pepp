#pragma once
#include <QtCore/QtCore>
#include "help2_globals.hpp"

struct HELP2_EXPORT HelpEntry {
  enum class Category {
    Root,
    About,
    Figure,
    ISAGreenCard,
    Text,
  } category;
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
  QList<QSharedPointer<HelpEntry>> children = {};
};
