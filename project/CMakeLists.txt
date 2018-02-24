cmake_minimum_required(VERSION 3.10)

option(ENABLE_TESTING "Whether to build the test harness and enable testing." FALSE)

if(ENABLE_TESTING)
    find_package(benchmark)
    set(Boost_USE_STATIC_LIBS TRUE)
    find_package(Boost COMPONENTS filesystem program_options unit_test_framework)
else()
    find_package(Boost)
endif()
find_path(GSL_INCLUDE_DIR gsl/gsl)

add_executable(
    cpplox

    src/main.cpp
    src/ast_printer.cpp
    src/expression.cpp
    src/interpreter.cpp
    src/parser.cpp
    src/scanner.cpp
    src/token.cpp
)
target_compile_features(cpplox PRIVATE cxx_std_14)
target_link_libraries(cpplox PRIVATE Boost::boost)
target_include_directories(cpplox PRIVATE "${GSL_INCLUDE_DIR}")
install(TARGETS cpplox DESTINATION ./)

# CMake doesn't abstract warning options for us, so detect compiler. Assume anything that isn't MSVC is GNU or GNU-compatible.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # /Wall is broken in MSVC 2017, so use /W4 instead
    # (https://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/891a02d2-d0cf-495a-bcea-41001cf599de/)
    target_compile_options(cpplox PRIVATE /W4)
else()
    target_compile_options(cpplox PRIVATE -Wall -Wextra)
endif()

# Disabling RTTI because I don't need it for this program, and it's probably a smell if I did.
# CMake doesn't abstract RTTI options for us, so detect compiler. Assume anything that isn't MSVC is GNU or GNU-compatible.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(cpplox PRIVATE /GR-)
else()
    target_compile_options(cpplox PRIVATE -fno-rtti)
endif()

if(ENABLE_TESTING)
    add_executable(test_harness test/main.cpp)
    target_compile_features(test_harness PRIVATE cxx_std_14)
    target_link_libraries(test_harness PRIVATE Boost::boost Boost::filesystem Boost::program_options Boost::unit_test_framework)

    add_executable(bench_harness bench/main.cpp)
    target_compile_features(bench_harness PRIVATE cxx_std_14)
    target_link_libraries(bench_harness PRIVATE benchmark::benchmark Boost::boost Boost::filesystem Boost::program_options)

    enable_testing()
    add_test(
        NAME test_harness
        COMMAND test_harness -- --cpplox-file "$<TARGET_FILE:cpplox>" --test-scripts-path "${CMAKE_CURRENT_SOURCE_DIR}/test/scripts"
    )
    add_custom_target(
        bench
        bench_harness --cpplox-file "$<TARGET_FILE:cpplox>" --test-scripts-path "${CMAKE_CURRENT_SOURCE_DIR}/test/scripts"
        DEPENDS bench_harness cpplox
    )

    find_program(VALGRIND_COMMAND valgrind)
    message(STATUS "Check for valgrind: ${VALGRIND_COMMAND}")
    if(NOT VALGRIND_COMMAND STREQUAL "VALGRIND_COMMAND-NOTFOUND")
        add_test(
            NAME valgrind
            COMMAND "${VALGRIND_COMMAND}" "$<TARGET_FILE:cpplox>" "${CMAKE_CURRENT_SOURCE_DIR}/test/scripts/evaluate.lox"
        )
        set_tests_properties(
            valgrind PROPERTIES
            PASS_REGULAR_EXPRESSION "All heap blocks were freed -- no leaks are possible"
        )
    endif()
endif()
