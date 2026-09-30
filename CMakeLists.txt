cmake_minimum_required(VERSION 3.21)
project(cwASIO)

add_subdirectory(src)
# Only build the tests when cwASIO is the top level project, not when included via FetchContent.
if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()
