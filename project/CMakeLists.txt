cmake_minimum_required(VERSION 3.10)

option(ENABLE_TESTING "Whether to build the test and bench harness and enable testing." FALSE)

find_package(Boost)
find_path(GSL_INCLUDE_DIR gsl/gsl)
find_path(HBOEHM_GC_INSTALL_DIR include/gc.h)
if(ENABLE_TESTING)
    find_package(benchmark)
    set(Boost_USE_STATIC_LIBS TRUE)
    find_package(Boost COMPONENTS filesystem program_options unit_test_framework)
endif()

add_executable(cpplox
    src/main.cpp
    src/ast_printer.cpp
    src/class.cpp
    src/environment.cpp
    src/expression.cpp
    src/expression_impls.cpp
    src/function.cpp
    src/interpreter.cpp
    src/literal.cpp
    src/parser.cpp
    src/resolver.cpp
    src/scanner.cpp
    src/statement_impls.cpp
    src/token.cpp
)
target_compile_features(cpplox PRIVATE cxx_std_14)
target_link_libraries(cpplox PRIVATE
    Boost::boost
    "${HBOEHM_GC_INSTALL_DIR}/lib/$<IF:$<BOOL:${CMAKE_HOST_UNIX}>,libgc.a,gc-lib.lib>"
    $<$<BOOL:${CMAKE_HOST_UNIX}>:pthread>
)
target_include_directories(cpplox PRIVATE "${GSL_INCLUDE_DIR}" "${HBOEHM_GC_INSTALL_DIR}/include")
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

if(ENABLE_TESTING)
    add_executable(test_harness test/main.cpp)
    target_compile_features(test_harness PRIVATE cxx_std_14)
    target_link_libraries(test_harness PRIVATE
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

    # find_program(VALGRIND_COMMAND valgrind)
    # message(STATUS "Check for valgrind: ${VALGRIND_COMMAND}")
    # if(NOT VALGRIND_COMMAND STREQUAL "VALGRIND_COMMAND-NOTFOUND")
    #     add_test(
    #         NAME valgrind
    #         COMMAND "${VALGRIND_COMMAND}" "$<TARGET_FILE:cpplox>" "${CMAKE_CURRENT_SOURCE_DIR}/bench/scripts/fib.lox"
    #     )
    #     set_tests_properties(valgrind PROPERTIES PASS_REGULAR_EXPRESSION "ERROR SUMMARY: 0 errors from 0 contexts")
    # endif()

    add_executable(bench_harness bench/main.cpp)
    target_compile_features(bench_harness PRIVATE cxx_std_14)
    target_link_libraries(bench_harness PRIVATE
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
            --test-scripts-path "${CMAKE_CURRENT_SOURCE_DIR}/bench/scripts"
            --jlox-file "$<IF:$<STREQUAL:${JLOX_RUN_SCRIPT},JLOX_RUN_SCRIPT-NOTFOUND>,\"\",${JLOX_RUN_SCRIPT}>"
            --node-file "$<IF:$<STREQUAL:${NODE_COMMAND},NODE_COMMAND-NOTFOUND>,\"\",${NODE_COMMAND}>"
        DEPENDS bench_harness cpplox
    )
endif()
