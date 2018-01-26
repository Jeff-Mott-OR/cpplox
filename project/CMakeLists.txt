cmake_minimum_required(VERSION 3.10)

find_package(Boost)
find_path(GSL_INCLUDE_DIR gsl/gsl)

add_executable(
    main

    src/main.cpp
    src/ast_printer.cpp
    src/expression.cpp
    src/scanner.cpp
    src/token.cpp
)
set_property(TARGET main PROPERTY CXX_STANDARD 14)
target_link_libraries(main PRIVATE Boost::boost)
target_include_directories(main PRIVATE "${GSL_INCLUDE_DIR}")

# CMake doesn't abstract warning options for us, so detect compiler.
# Assume anything that isn't MSVC is GNU or GNU-compatible.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # /Wall is broken in MSVC 2017, so use /W4 instead
    # https://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/891a02d2-d0cf-495a-bcea-41001cf599de/
    target_compile_options(main PRIVATE /W4)
else()
    target_compile_options(main PRIVATE -Wall -Wextra)
endif()

# Disabling RTTI because I don't need it for this program, and it's probably a smell if I did.
# CMake doesn't abstract RTTI options for us, so detect compiler.
# Assume anything that isn't MSVC is GNU or GNU-compatible.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(main PRIVATE /GR-)
else()
    target_compile_options(main PRIVATE -fno-rtti)
endif()
