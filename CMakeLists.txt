cmake_minimum_required(VERSION 3.14)

set(BUILD_TESTING OFF)
set(BUILD_BENCHES ON)

project(gES LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_library(ges INTERFACE)

target_sources(ges PRIVATE 
  include/ges/core.hpp
  include/ges/dispatcher.hpp
  include/ges/event_info.hpp
  include/ges/event_queue.hpp
  include/ges/arena.hpp
  include/ges/viewer.hpp
  include/ges/batcher.hpp
  include/ges/delegate.hpp)

target_include_directories(ges INTERFACE external/meta-quick/include INTERFACE include)

add_subdirectory(external/meta-quick)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(BUILD_BENCHES)
  add_subdirectory(benches)



endif()