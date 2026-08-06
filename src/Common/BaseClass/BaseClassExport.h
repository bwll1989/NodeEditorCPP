#pragma once

#if defined(BASECLASS_LIBRARY)
#  define BASECLASS_EXPORT Q_DECL_EXPORT
#else
#  define BASECLASS_EXPORT Q_DECL_IMPORT
#endif
