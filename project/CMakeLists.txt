cmake_minimum_required(VERSION 3.10)

option(ENABLE_TESTING "Whether to build the test and bench harness and enable testing." FALSE)

find_package(Boost)
find_path(GSL_INCLUDE_DIR gsl/gsl)
find_path(GCPP_INCLUDE_DIR deferred_heap.h)
if(ENABLE_TESTING)
    find_package(benchmark)
    set(Boost_USE_STATIC_LIBS TRUE)
    find_package(Boost COMPONENTS filesystem program_options unit_test_framework)
endif()

add_executable(
    cpplox
        src/treewalk_interpreter/main.cpp
        src/treewalk_interpreter/ast_printer.cpp
        src/treewalk_interpreter/class.cpp
        src/treewalk_interpreter/environment.cpp
        src/treewalk_interpreter/expression.cpp
        src/treewalk_interpreter/expression_impls.cpp
        src/treewalk_interpreter/function.cpp
        src/treewalk_interpreter/interpreter.cpp
        src/treewalk_interpreter/literal.cpp
        src/treewalk_interpreter/parser.cpp
        src/treewalk_interpreter/resolver.cpp
        src/treewalk_interpreter/scanner.cpp
        src/treewalk_interpreter/statement_impls.cpp
        src/treewalk_interpreter/token.cpp
)
target_compile_features(cpplox PRIVATE cxx_std_14)
target_link_libraries(cpplox PRIVATE Boost::boost)
target_include_directories(cpplox PRIVATE "${GSL_INCLUDE_DIR}" "${GCPP_INCLUDE_DIR}")
install(TARGETS cpplox DESTINATION ./)

# CMake doesn't abstract warning options for us, so detect compiler. Assume
# anything that isn't MSVC is GNU or GNU- compatible.
#
# /Wall is broken in MSVC 2017, so use /W4 instead
# (https://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/891a02d2-d0cf-495a-bcea-41001cf599de/)
#
# We use some MSVC pragmas, which of course will be unknown to GCC and Clang
target_compile_options(cpplox PRIVATE "$<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra;-Wno-unknown-pragmas>")

# Disabling RTTI because I don't need it for this program, and it's probably a smell if I did.
#
# CMake doesn't abstract RTTI options for us, so detect compiler. Assume
# anything that isn't MSVC is GNU or GNU- compatible.
target_compile_options(cpplox PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/GR-,-fno-rtti>)

# Bytecode VM
add_executable(
    cpploxbc
        src/bytecode_vm/main.cpp
        src/bytecode_vm/chunk.cpp
        src/bytecode_vm/compiler.cpp
        src/bytecode_vm/debug.cpp
        src/bytecode_vm/memory.cpp
        src/bytecode_vm/object.cpp
        src/bytecode_vm/scanner.cpp
        src/bytecode_vm/table.cpp
        src/bytecode_vm/value.cpp
        src/bytecode_vm/vm.cpp
)
target_compile_features(cpploxbc PRIVATE cxx_std_14)
install(TARGETS cpploxbc DESTINATION ./)
target_compile_options(cpploxbc PRIVATE "$<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra;-Wno-unknown-pragmas>")
target_compile_options(cpploxbc PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/GR-,-fno-rtti>)

if(ENABLE_TESTING)
    add_executable(test_harness test/main.cpp)
    target_compile_features(test_harness PRIVATE cxx_std_14)
    target_link_libraries(
        test_harness
        PRIVATE
            Boost::boost
            Boost::filesystem
            Boost::program_options
            Boost::unit_test_framework
    )
    enable_testing()
    add_test(
        NAME test_harness
        COMMAND test_harness --
            --cpplox-file "$<TARGET_FILE:cpplox>"
            --test-scripts-path "${CMAKE_CURRENT_SOURCE_DIR}/test/scripts"
    )
    add_test(
        NAME test_harness_bc
        COMMAND test_harness --
            --cpplox-file "$<TARGET_FILE:cpploxbc>"
            --test-scripts-path "${CMAKE_CURRENT_SOURCE_DIR}/test/scripts"
    )

    find_program(VALGRIND_COMMAND valgrind)
    message(STATUS "Check for valgrind: ${VALGRIND_COMMAND}")
    if(NOT VALGRIND_COMMAND STREQUAL "VALGRIND_COMMAND-NOTFOUND")
        add_test(
            NAME valgrind
            COMMAND "${VALGRIND_COMMAND}" "$<TARGET_FILE:cpplox>" "${CMAKE_CURRENT_SOURCE_DIR}/bench/scripts/fib.lox"
        )
        set_tests_properties(valgrind PROPERTIES PASS_REGULAR_EXPRESSION "ERROR SUMMARY: 0 errors from 0 contexts")

        add_test(
            NAME valgrind_bc
            COMMAND "${VALGRIND_COMMAND}" "$<TARGET_FILE:cpploxbc>" "${CMAKE_CURRENT_SOURCE_DIR}/bench/scripts/fib.lox"
        )
        set_tests_properties(valgrind_bc PROPERTIES PASS_REGULAR_EXPRESSION "ERROR SUMMARY: 0 errors from 0 contexts")
    endif()

    add_executable(bench_harness bench/main.cpp)
    target_compile_features(bench_harness PRIVATE cxx_std_14)
    target_link_libraries(
        bench_harness
        PRIVATE
            benchmark::benchmark
            Boost::boost
            Boost::filesystem
            Boost::program_options
    )

    find_file(JLOX_RUN_SCRIPT jlox_run.cmake)
    message(STATUS "Check for jlox: ${JLOX_RUN_SCRIPT}")

    find_program(NODE_COMMAND node)
    message(STATUS "Check for node: ${NODE_COMMAND}")

    add_custom_target(
        bench
        bench_harness
            --cpplox-file "$<TARGET_FILE:cpplox>"
            --cpploxbc-file "$<TARGET_FILE:cpploxbc>"
            --test-scripts-path "${CMAKE_CURRENT_SOURCE_DIR}/bench/scripts"
            --jlox-file "$<IF:$<STREQUAL:${JLOX_RUN_SCRIPT},JLOX_RUN_SCRIPT-NOTFOUND>,\"\",${JLOX_RUN_SCRIPT}>"
            --node-file "$<IF:$<STREQUAL:${NODE_COMMAND},NODE_COMMAND-NOTFOUND>,\"\",${NODE_COMMAND}>"
        DEPENDS bench_harness cpplox
    )
endif()
