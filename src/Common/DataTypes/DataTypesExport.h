#pragma once
#if defined(DATATYPES_LIBRARY)
#  define DATATYPES_EXPORT __declspec(dllexport)
#else
#  define DATATYPES_EXPORT __declspec(dllimport)
#endif