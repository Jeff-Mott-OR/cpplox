cmake_minimum_required(VERSION 3.10)

file(GLOB TOOL_SOURCES "${CMAKE_CURRENT_LIST_DIR}/java/com/craftinginterpreters/tool/*.java")
string(REPLACE ";" "\" \"" TOOL_SOURCES_STR "\"${TOOL_SOURCES}\"")
exec_program(javac ARGS "-cp \"${CMAKE_CURRENT_LIST_DIR}/java\" -d \"${CMAKE_CURRENT_LIST_DIR}/build/java\" -Werror -implicit:none ${TOOL_SOURCES_STR}")
exec_program(java ARGS "-cp \"${CMAKE_CURRENT_LIST_DIR}/build/java\" com.craftinginterpreters.tool.GenerateAst \"${CMAKE_CURRENT_LIST_DIR}/java/com/craftinginterpreters/lox\"")

file(GLOB LOX_SOURCES "${CMAKE_CURRENT_LIST_DIR}/java/com/craftinginterpreters/lox/*.java")
string(REPLACE ";" "\" \"" LOX_SOURCES_STR "\"${LOX_SOURCES}\"")
exec_program(javac ARGS "-cp \"${CMAKE_CURRENT_LIST_DIR}/java\" -d \"${CMAKE_CURRENT_LIST_DIR}/build/java\" -Werror -implicit:none ${LOX_SOURCES_STR}")
