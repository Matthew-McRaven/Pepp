#pragma once

#include <QtCore/QtGlobal>

#if defined(BUILTINS_LIBRARY)
  #define BUILTINS_EXPORT Q_DECL_EXPORT
#else
  #define BUILTINS_EXPORT Q_DECL_IMPORT
#endif
