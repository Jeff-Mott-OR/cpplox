cmake_minimum_required(VERSION 3.21)
project(Project)

include(FetchContent)

FetchContent_Declare(
    Microsoft_GSL
    GIT_REPOSITORY https://github.com/microsoft/GSL
    GIT_TAG v4.0.0
    GIT_SHALLOW ON
)

set(BOOST_INCLUDE_LIBRARIES program_options)
FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.80.0
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(Microsoft_GSL Boost)

add_executable(project main.cpp)
target_compile_features(project PUBLIC cxx_std_20)
set_property(TARGET project PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
target_link_libraries(project PRIVATE Microsoft.GSL::GSL Boost::program_options)
