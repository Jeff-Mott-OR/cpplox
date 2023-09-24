cmake_minimum_required(VERSION 3.21)
project(Project)

option(DEPS_ONLY "Fetch dependencies only" FALSE)
option(ENABLE_TESTING "Build and run tests" FALSE)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(BOOST_INCLUDE_LIBRARIES algorithm convert process program_options test)
FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.80.0
    GIT_SHALLOW ON
)

FetchContent_Declare(
    Microsoft_GSL
    GIT_REPOSITORY https://github.com/microsoft/GSL
    GIT_TAG v4.0.0
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(Boost Microsoft_GSL)

if(NOT DEPS_ONLY)
    add_executable(cpploxbc src/main.cpp src/scanner.cpp src/compiler.cpp src/vm.cpp)
    target_compile_features(cpploxbc PUBLIC cxx_std_20)
    target_compile_options(cpploxbc PRIVATE -Wall -Wextra -Werror)
    set_property(TARGET cpploxbc PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
    target_link_libraries(cpploxbc PRIVATE Boost::algorithm Boost::convert Boost::program_options Microsoft.GSL::GSL)

    if(ENABLE_TESTING)
        enable_testing()
        add_test(NAME cppcheck COMMAND cppcheck --error-exitcode=1 "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/compiler.cpp")

        add_executable(scanner_test test/scanner-test.cpp src/scanner.cpp)
        target_compile_features(scanner_test PUBLIC cxx_std_20)
        target_compile_options(scanner_test PRIVATE -Wall -Wextra -Werror)
        set_property(TARGET scanner_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
        target_link_libraries(scanner_test PRIVATE
            Boost::unit_test_framework
            # Project dependencies
            Boost::algorithm Boost::convert Boost::program_options Microsoft.GSL::GSL
        )
        add_test(NAME scanner_test COMMAND scanner_test)

        add_executable(compiler_test test/compiler-test.cpp src/compiler.cpp src/scanner.cpp)
        target_compile_features(compiler_test PUBLIC cxx_std_20)
        target_compile_options(compiler_test PRIVATE -Wall -Wextra -Werror)
        set_property(TARGET compiler_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
        target_link_libraries(compiler_test PRIVATE
            Boost::unit_test_framework
            # Project dependencies
            Boost::algorithm Boost::convert Boost::program_options Microsoft.GSL::GSL
        )
        add_test(NAME compiler_test COMMAND compiler_test)

        add_executable(vm_test test/vm-test.cpp src/vm.cpp src/compiler.cpp src/scanner.cpp)
        target_compile_features(vm_test PUBLIC cxx_std_20)
        target_compile_options(vm_test PRIVATE -Wall -Wextra -Werror)
        set_property(TARGET vm_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
        target_link_libraries(vm_test PRIVATE
            Boost::unit_test_framework
            # Project dependencies
            Boost::algorithm Boost::convert Boost::program_options Microsoft.GSL::GSL
        )
        add_test(NAME vm_test COMMAND vm_test)

        add_executable(cli_test test/cli-test.cpp)
        target_compile_features(cli_test PUBLIC cxx_std_20)
        target_compile_options(cli_test PRIVATE -Wall -Wextra -Werror)
        set_property(TARGET cli_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
        target_link_libraries(cli_test PRIVATE Boost::process Boost::unit_test_framework)
        add_test(NAME cli_test COMMAND cli_test)
    endif()
endif()
