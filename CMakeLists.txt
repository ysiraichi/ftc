cmake_minimum_required (VERSION 2.6)
project (FTC-1.0)

include_directories (include)

find_package (BISON)
find_package (FLEX)

set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
set (CMAKE_C_FLAGS "-g")

add_subdirectory (src)
