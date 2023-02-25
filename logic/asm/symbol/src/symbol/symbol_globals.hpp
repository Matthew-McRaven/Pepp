#pragma once

#include <QtCore/QtGlobal>

#if defined(SYMBOL_LIBRARY)
  #define SYMBOL_EXPORT Q_DECL_EXPORT
#else
  #define SYMBOL_EXPORT Q_DECL_IMPORT
#endif
