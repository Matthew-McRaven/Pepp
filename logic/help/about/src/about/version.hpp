#pragma once
namespace about {
const char* const g_GIT_SHA1();
const char* const g_GIT_TAG();
extern const int g_MAJOR_VERSION;
extern const int g_MINOR_VERSION;
extern const int g_PATCH_VERSION;
extern const bool g_GIT_LOCAL_CHANGES;
} // namespace about
