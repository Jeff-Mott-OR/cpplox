cmake_minimum_required(VERSION 3.10)

set(Boost_USE_STATIC_LIBS TRUE)
find_package(Boost COMPONENTS filesystem unit_test_framework)

find_path(GSL_INCLUDE_DIR gsl/gsl)

add_executable(
    cpplox

    src/main.cpp
    src/ast_printer.cpp
    src/expression.cpp
    src/parser.cpp
    src/scanner.cpp
    src/token.cpp
)
set_property(TARGET cpplox PROPERTY CXX_STANDARD 14)
target_link_libraries(cpplox PRIVATE Boost::boost)
target_include_directories(cpplox PRIVATE "${GSL_INCLUDE_DIR}")

# Install into wherever CMAKE_INSTALL_PREFIX points to
install(TARGETS cpplox DESTINATION ./)

# CMake doesn't abstract warning options for us, so detect compiler.
# Assume anything that isn't MSVC is GNU or GNU-compatible.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # /Wall is broken in MSVC 2017, so use /W4 instead
    # https://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/891a02d2-d0cf-495a-bcea-41001cf599de/
    target_compile_options(cpplox PRIVATE /W4)
else()
    target_compile_options(cpplox PRIVATE -Wall -Wextra)
endif()

# Disabling RTTI because I don't need it for this program, and it's probably a smell if I did.
# CMake doesn't abstract RTTI options for us, so detect compiler.
# Assume anything that isn't MSVC is GNU or GNU-compatible.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(cpplox PRIVATE /GR-)
else()
    target_compile_options(cpplox PRIVATE -fno-rtti)
endif()

option(ENABLE_TESTING "Whether to build the test harness and enable testing." FALSE)
if(ENABLE_TESTING)
    add_executable(test_harness test/main.cpp)
    set_property(TARGET test_harness PROPERTY CXX_STANDARD 14)
    target_link_libraries(test_harness PRIVATE Boost::boost Boost::filesystem Boost::unit_test_framework)
    target_include_directories(test_harness PRIVATE "${GSL_INCLUDE_DIR}")

    # Install into wherever CMAKE_INSTALL_PREFIX points to
    install(TARGETS test_harness DESTINATION ./test)
    install(DIRECTORY test/scripts DESTINATION ./test)

    # Just one test to kick-off the test harness
    enable_testing()
    add_test(NAME test_harness COMMAND test_harness -- "$<TARGET_FILE:cpplox>" WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/test")
endif()
