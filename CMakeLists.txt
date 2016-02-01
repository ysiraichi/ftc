cmake_minimum_required (VERSION 2.6)
project (FTC-1.0)

include_directories (include)

find_package (BISON)
find_package (FLEX)

execute_process (COMMAND 
  llvm-config --cflags 
  OUTPUT_VARIABLE CFLAGS_)
execute_process (COMMAND 
  llvm-config --cxxflags --ldflags --libs core --system-libs
  OUTPUT_VARIABLE CXXFLAGS_)

string (REPLACE "\n" " " CFLAGS ${CFLAGS_})
string (REPLACE "\n" " " CXXFLAGS ${CXXFLAGS_})

set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}")
set (CMAKE_C_FLAGS "-g ${CFLAGS}")
set (CMAKE_CXX_FLAGS "-g ${CXXFLAGS}")

add_subdirectory (src)
