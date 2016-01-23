set (FLEX_INPUT "LexFTiger")
set (BISON_INPUT "SynFTiger")

BISON_TARGET(Parser 
  "${BISON_INPUT}.y" 
  "${CMAKE_CURRENT_BINARY_DIR}/${BISON_INPUT}.c"
  COMPILE_FLAGS "-v")

configure_file ("${FLEX_INPUT}.l.in" "${CMAKE_CURRENT_BINARY_DIR}/${FLEX_INPUT}.l")

FLEX_TARGET(Scanner 
  "${CMAKE_CURRENT_BINARY_DIR}/${FLEX_INPUT}.l" 
  "${CMAKE_CURRENT_BINARY_DIR}/${FLEX_INPUT}.c")

ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

add_library (Analysis 
  ${FLEX_Scanner_OUTPUTS} 
  ${BISON_Parser_OUTPUTS}
  ParserTree.c 
  DrawTree.c)

target_link_libraries (Analysis Support ${FLEX_LIBRARIES})
