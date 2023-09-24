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
    add_executable(project src/main.cpp src/scanner.cpp)
    target_compile_features(project PUBLIC cxx_std_20)
    set_property(TARGET project PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
    target_link_libraries(project PRIVATE Microsoft.GSL::GSL Boost::algorithm Boost::program_options)
    target_compile_options(project PRIVATE -Wall -Wextra -Werror)

    if(ENABLE_TESTING)
        add_executable(project_test test/test.cpp src/scanner.cpp)
        target_compile_features(project_test PUBLIC cxx_std_20)
        set_property(TARGET project_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
        target_link_libraries(project_test PRIVATE Boost::unit_test_framework)
        target_compile_options(project_test PRIVATE -Wall -Wextra -Werror)

        enable_testing()
        add_test(NAME cppcheck COMMAND cppcheck --error-exitcode=1 "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.cpp")
        add_test(NAME project_test COMMAND project_test)
    endif()
endif()
