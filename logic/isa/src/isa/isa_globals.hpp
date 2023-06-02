#pragma once

#include <QtCore/QtGlobal>

#if defined(ISA_LIBRARY)
#define ISA_EXPORT Q_DECL_EXPORT
#else
#define ISA_EXPORT Q_DECL_IMPORT
#endif
