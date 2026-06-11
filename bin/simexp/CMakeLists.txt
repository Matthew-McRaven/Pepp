cmake_minimum_required(VERSION 3.16)

qt_add_executable(
  simexp
  main.cpp
  simuloop.cpp
  pep10isa.cpp
  event_dispatch.cpp
  event_queue.cpp
  event_allocator.cpp
  event_scheduler.cpp)

target_link_libraries(simexp PRIVATE pepp-core)
set_target_properties(simexp PROPERTIES FOLDER "qtc_runnable")
