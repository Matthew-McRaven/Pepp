cmake_minimum_required(VERSION 3.16)

project(PepNext VERSION 0.1 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(3rd-party)

find_package(Qt6 6.4 COMPONENTS Quick Core REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
add_subdirectory(logic)
