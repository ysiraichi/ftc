add_subdirectory (Analysis)
add_subdirectory (Support)

add_executable (ftc Main.c)
target_link_libraries (ftc Support Analysis)
install (TARGETS ftc RUNTIME DESTINATION bin)
