add_subdirectory (Analysis)
add_subdirectory (Support)
add_subdirectory (LLVMCode)

add_executable (ftc Main.c)

target_link_libraries (ftc 
  Support Analysis IRTranslation
  # LLVM Libraries (core target)
  LLVMTarget LLVMMC LLVMAnalysis LLVMCore 
  LLVMSupport
  # LLVM System Libraries
  rt dl tinfo pthread z m)

install (TARGETS ftc RUNTIME DESTINATION bin)

set_target_properties (ftc PROPERTIES LINKER_LANGUAGE CXX)
