cmake_minimum_required(VERSION 3.16)

qt_add_executable(
  simexp
  main.cpp
  pep10isa.cpp
  event_dispatch.cpp
  event_loop.cpp
  event_allocator.cpp
  event_scheduler.cpp
  sim_top.cpp
  sim_device.cpp
  sim_eventhandle.hpp sim_eventhandle.cpp)

target_link_libraries(simexp PRIVATE pepp-core)
set_target_properties(simexp PROPERTIES FOLDER "qtc_runnable")
