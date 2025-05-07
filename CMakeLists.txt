cmake_minimum_required(VERSION 3.14)

project(gES)

set(CMAKE_CXX_STANDARD 20)

add_library(ges)

target_sources(ges PRIVATE 
  src/event_queue.cpp
  include/ges/core.hpp
  include/ges/dispatcher.hpp
  include/ges/event_info.hpp
  include/ges/event_queue.hpp
  include/ges/arena.hpp
  include/ges/delegate.hpp)

target_include_directories(ges PUBLIC include)

target_link_libraries(ges PUBLIC meta-quick)

add_subdirectory(external/meta-quick)
add_subdirectory(tests)