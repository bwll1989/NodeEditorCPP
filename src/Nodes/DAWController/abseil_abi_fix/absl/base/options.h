// Shadow header: match prebuilt abseil_dll (std::string_view ABI).
// The install copy defaults to 0, which breaks linking against abseil_dll.lib.

#ifndef ABSL_BASE_OPTIONS_H_
#define ABSL_BASE_OPTIONS_H_

#define ABSL_OPTION_USE_STD_STRING_VIEW 1
#define ABSL_OPTION_USE_STD_ORDERING 0
#define ABSL_OPTION_USE_INLINE_NAMESPACE 1
#define ABSL_OPTION_INLINE_NAMESPACE_NAME lts_20250512
#define ABSL_OPTION_HARDENED 0

#endif  // ABSL_BASE_OPTIONS_H_
