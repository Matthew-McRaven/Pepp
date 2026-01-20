cmake_minimum_required(VERSION 3.24)
project(Scintilla LANGUAGES CXX)
# Needed so GENERATED is propogated See:
# https://cmake.org/cmake/help/latest/policy/CMP0118.html Discussion here:
# https://discourse.cmake.org/t/bug-with-generated-file-and-target-sources-private/5262/11
# Needs CMake 3.20, hence line 1.
cmake_policy(SET CMP0118 NEW)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT IOS)
  enable_testing()
endif()
find_package(
  Qt6 6.4
  COMPONENTS
  REQUIRED Quick Core Gui Qml Widgets)
qt_policy(SET QTP0001 NEW)
add_subdirectory(lexilla/)
add_subdirectory(scintilla/)
