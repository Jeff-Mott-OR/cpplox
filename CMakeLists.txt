cmake_minimum_required(VERSION 3.21)
project(Project)

option(DEPS_ONLY "Fetch dependencies only" FALSE)
option(ENABLE_TESTING "Build and run tests" FALSE)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    Microsoft_GSL
    GIT_REPOSITORY https://github.com/microsoft/GSL
    GIT_TAG v4.0.0
    GIT_SHALLOW ON
)

set(BOOST_INCLUDE_LIBRARIES algorithm program_options test)
FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.80.0
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(Microsoft_GSL Boost)

if(NOT DEPS_ONLY)
    add_executable(project src/main.cpp src/scanner.cpp src/compiler.cpp)
    target_compile_features(project PUBLIC cxx_std_20)
    target_compile_options(project PRIVATE -Wall -Wextra -Werror)
    set_property(TARGET project PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
    target_link_libraries(project PRIVATE Microsoft.GSL::GSL Boost::algorithm Boost::program_options)

    if(ENABLE_TESTING)
        enable_testing()
        add_test(NAME cppcheck COMMAND cppcheck --error-exitcode=1 "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/compiler.cpp")

        add_executable(scanner_test test/scanner-test.cpp src/scanner.cpp)
        target_compile_features(scanner_test PUBLIC cxx_std_20)
        target_compile_options(scanner_test PRIVATE -Wall -Wextra -Werror)
        set_property(TARGET scanner_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
        target_link_libraries(scanner_test PRIVATE Boost::unit_test_framework)
        add_test(NAME scanner_test COMMAND scanner_test)

        add_executable(compiler_test test/compiler-test.cpp src/compiler.cpp)
        target_compile_features(compiler_test PUBLIC cxx_std_20)
        target_compile_options(compiler_test PRIVATE -Wall -Wextra -Werror)
        set_property(TARGET compiler_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
        target_link_libraries(compiler_test PRIVATE Boost::unit_test_framework Microsoft.GSL::GSL)
        add_test(NAME compiler_test COMMAND compiler_test)
    endif()
endif()
