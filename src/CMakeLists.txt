add_subdirectory (Analysis)
add_subdirectory (Support)
add_subdirectory (LLVMCode)

add_executable (ftc Main.c)

target_link_libraries (ftc 
  Support Analysis IRTranslation
  # LLVM Libraries (core target)
  LLVMBPFCodeGen LLVMBPFDesc LLVMBPFInfo LLVMBPFAsmPrinter LLVMAMDGPUCodeGen 
  LLVMipo LLVMVectorize LLVMAMDGPUAsmParser LLVMAMDGPUUtils LLVMAMDGPUDesc 
  LLVMAMDGPUInfo LLVMAMDGPUAsmPrinter LLVMSystemZDisassembler LLVMSystemZCodeGen 
  LLVMSystemZAsmParser LLVMSystemZDesc LLVMSystemZInfo LLVMSystemZAsmPrinter 
  LLVMHexagonDisassembler LLVMHexagonCodeGen LLVMHexagonDesc LLVMHexagonInfo 
  LLVMNVPTXCodeGen LLVMNVPTXDesc LLVMNVPTXInfo LLVMNVPTXAsmPrinter 
  LLVMCppBackendCodeGen LLVMCppBackendInfo LLVMMSP430CodeGen LLVMMSP430Desc 
  LLVMMSP430Info LLVMMSP430AsmPrinter LLVMXCoreDisassembler LLVMXCoreCodeGen 
  LLVMXCoreDesc LLVMXCoreInfo LLVMXCoreAsmPrinter LLVMMipsDisassembler LLVMMipsCodeGen 
  LLVMMipsAsmParser LLVMMipsDesc LLVMMipsInfo LLVMMipsAsmPrinter LLVMAArch64Disassembler 
  LLVMAArch64CodeGen LLVMAArch64AsmParser LLVMAArch64Desc LLVMAArch64Info 
  LLVMAArch64AsmPrinter LLVMAArch64Utils LLVMARMDisassembler LLVMARMCodeGen LLVMARMAsmParser 
  LLVMARMDesc LLVMARMInfo LLVMARMAsmPrinter LLVMPowerPCDisassembler LLVMPowerPCCodeGen 
  LLVMPowerPCAsmParser LLVMPowerPCDesc LLVMPowerPCInfo LLVMPowerPCAsmPrinter 
  LLVMSparcDisassembler LLVMSparcCodeGen LLVMSparcAsmParser LLVMSparcDesc LLVMSparcInfo 
  LLVMSparcAsmPrinter LLVMX86Disassembler LLVMX86AsmParser LLVMX86CodeGen LLVMSelectionDAG 
  LLVMAsmPrinter LLVMCodeGen LLVMTarget LLVMScalarOpts LLVMProfileData LLVMInstCombine 
  LLVMInstrumentation LLVMTransformUtils LLVMipa LLVMAnalysis LLVMX86Desc LLVMObject 
  LLVMMCParser LLVMBitReader LLVMMCDisassembler LLVMX86Info LLVMX86AsmPrinter LLVMMC
  LLVMX86Utils LLVMCore LLVMSupport
  # LLVM System Libraries
  rt dl tinfo pthread z m)

install (TARGETS ftc RUNTIME DESTINATION bin)

set_target_properties (ftc PROPERTIES LINKER_LANGUAGE CXX)
