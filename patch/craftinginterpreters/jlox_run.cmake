cmake_minimum_required(VERSION 3.10)

exec_program(java ARGS "-cp \"${CMAKE_CURRENT_LIST_DIR}/build/java\" com.craftinginterpreters.lox.Lox \"${CMAKE_ARGV3}\"")
