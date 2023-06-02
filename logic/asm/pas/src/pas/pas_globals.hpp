#pragma once

#include <QtCore/QtGlobal>

#if defined(PAS_LIBRARY)
#define PAS_EXPORT Q_DECL_EXPORT
#else
#define PAS_EXPORT Q_DECL_IMPORT
#endif
