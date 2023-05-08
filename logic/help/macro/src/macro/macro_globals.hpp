#pragma once

#include <QtCore/QtGlobal>

#if defined(MACRO_LIBRARY)
  #define MACRO_EXPORT Q_DECL_EXPORT
#else
  #define MACRO_EXPORT Q_DECL_IMPORT
#endif
